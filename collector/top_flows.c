
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>  // For sleep
#include "top_flows.h"
#include "./libgeohash/geohash.h"  // Include your geohash library header

#define TOP_N 100
#define NUM_BINS 20
#define BIN_SIZE 10000
#define BIN_SIZE_RTT 10000000

void update_total_flows(redisContext *c, int total) {
    const char *hash_key = "flow_metrics"; // Hash to store total flows
    const char *field_name = "total_flows"; // Field within the hash

    // Increment the total flows
    redisReply *reply = redisCommand(c, "HSET %s %s %d", hash_key, field_name, total);
    if (reply == NULL) {
        fprintf(stderr, "Error executing HINCRBY: %s\n", c->errstr);
        return; // Exit if there's an error
    }
    
    freeReplyObject(reply);
}



void update_total_flows_bytes(redisContext *c, long int total) {
    static long int prev_total_bytes = 0;  // Store previous total bytes
    static struct timespec prev_time = {0}; // Store previous timestamp

    const char *hash_key = "flow_metrics";  // Hash to store total flows
    const char *field_bytes = "total_bytes";  // Field within the hash for total bytes
    const char *field_throughput = "throughput";  // Field within the hash for throughput

    // Get the current timestamp in seconds
    struct timespec current_time;
    clock_gettime(CLOCK_REALTIME, &current_time);

    // Calculate throughput based on the difference between the current and previous total bytes
    double throughput_bps = 0.0;
    if (prev_time.tv_sec != 0) {
        long int bytes_diff = total - prev_total_bytes;
        long int time_diff = current_time.tv_sec - prev_time.tv_sec;

        if (time_diff > 0) {
            throughput_bps = (double)(bytes_diff * 8) / time_diff;  // Convert to bits per second
        }
    }

    // Update the total bytes and throughput in Redis
    redisReply *reply = redisCommand(c, "HSET %s %s %ld", hash_key, field_bytes, total);
    if (reply == NULL) {
        fprintf(stderr, "Error updating total bytes in Redis: %s\n", c->errstr);
        return;  // Exit if there's an error
    }
    freeReplyObject(reply);


    if(throughput_bps < 10000000000)
    {
        reply = redisCommand(c, "HSET %s %s %.2f", hash_key, field_throughput, throughput_bps);
        if (reply == NULL) {
            fprintf(stderr, "Error updating throughput in Redis: %s\n", c->errstr);
            return;  // Exit if there's an error
        }
        freeReplyObject(reply);
    }
    // Update previous values for the next iteration
    prev_total_bytes = total;
    prev_time = current_time;
}



void push_flow_to_redis(redisContext *c, HashNode *node) {
    char key[128];
    char geo_key[128];
    long long timestamp = (long long) time(NULL) * 1000; // Current time in milliseconds
    int gai_error, mmdb_error;
    MMDB_lookup_result_s result = MMDB_lookup_string(&mmdb, ip_to_string(node->payload.src_ip), &gai_error, &mmdb_error);

    double latitude = 0.0;
    double longitude = 0.0;
    char* geo_hash = NULL;  // Buffer to store geohash string

    if (result.found_entry) {
        MMDB_entry_data_s entry_data;

        // Look up the latitude
        int lookup_status_lat = MMDB_get_value(&result.entry, &entry_data, "location", "latitude", NULL);
        if (lookup_status_lat == MMDB_SUCCESS && entry_data.has_data && entry_data.type == MMDB_DATA_TYPE_DOUBLE) {
            latitude = entry_data.double_value;
        } 

        // Look up the longitude
        int lookup_status_lon = MMDB_get_value(&result.entry, &entry_data, "location", "longitude", NULL);
        if (lookup_status_lon == MMDB_SUCCESS && entry_data.has_data && entry_data.type == MMDB_DATA_TYPE_DOUBLE) {
            longitude = entry_data.double_value;
        } 
    
        if (longitude != 0 && latitude != 0) {
            geo_hash = geohash_encode(latitude, longitude, 11);
        }
    }

    snprintf(key, sizeof(key), "top_flows");
    snprintf(geo_key, sizeof(geo_key), "geo_hashes");

    // Add the source IP to the sorted set with accumulated bytes as the score
    redisReply *reply = redisCommand(c, "ZADD %s %lu %s", key, node->accumulated_bytes, ip_to_string(node->payload.src_ip));
    
    // Store the geohash in a separate hash
    if (geo_hash) {
        redisCommand(c, "HSET %s %s %s", geo_key, ip_to_string(node->payload.src_ip), geo_hash);
    }

    // Check for errors
    if (reply == NULL) {
        fprintf(stderr, "Error executing ZADD: %s\n", c->errstr);
    } else {
        freeReplyObject(reply);
    }
}


void maintain_top_flows(redisContext *c) {
    // Get the count of the sorted set
    redisReply *reply_count = redisCommand(c, "DEL top_flows  geo_hashes");
    freeReplyObject(reply_count);
}

int compare_by_accumulated_bytes(const void* a, const void* b) {
    HashNode* node_a = *(HashNode**)a;
    HashNode* node_b = *(HashNode**)b;
    
    // Sort in descending order
    if (node_a->accumulated_bytes < node_b->accumulated_bytes) return 1;
    if (node_a->accumulated_bytes > node_b->accumulated_bytes) return -1;
    return 0;
}

void create_histogram(redisContext *c) {
    if (c == NULL) {
        fprintf(stderr, "Redis context is NULL\n");
        return;
    }

    // Define histogram bins
    int histogram_bins[NUM_BINS + 1] = {0};  // Extra bin for overflow
    int overflow_count = 0;  // To count values that exceed the max bin

    // Lock the hash table for safe iteration (if multithreading)
    // pthread_mutex_lock(&hash_table_mutex);

    // Iterate over the hash table to populate histogram bins
    for (int i = 0; i < TABLE_SIZE; i++) {
        HashNode* node = table->table[i];
        if (node != NULL && node->accumulated_bytes > 0) {
            int bin_index = node->accumulated_bytes / BIN_SIZE;
            if (bin_index < NUM_BINS) {
                histogram_bins[bin_index]++;
            } else {
                overflow_count++;  // Increment overflow count
            }
        }
    }

    // Store the overflow count in the last bin
    histogram_bins[NUM_BINS] = overflow_count;

    // Unlock the hash table after iteration
    // pthread_mutex_unlock(&hash_table_mutex);

    // Store histogram data in a Redis hash
    char key[128];
    snprintf(key, sizeof(key), "histogram:accumulated_bytes");
    for (int i = 0; i <= NUM_BINS; i++) {  // Include the overflow bin
        redisReply *reply_hset = redisCommand(c, "HSET %s %d %d", key, i, histogram_bins[i]);
        if (reply_hset == NULL) {
            fprintf(stderr, "Error updating histogram data in Redis: %s\n", c->errstr);
        } else {
            freeReplyObject(reply_hset);
        }
    }
}


void create_rtt_histogram(redisContext *c) {
    if (c == NULL) {
        fprintf(stderr, "Redis context is NULL\n");
        return;
    }

    // Define histogram bins
    int rtt_histogram_bins[NUM_BINS + 1] = {0};  // Extra bin for overflow
    int overflow_count = 0;  // To count RTT values that exceed the max bin

    // Lock the hash table for safe iteration (if multithreading)
    // pthread_mutex_lock(&hash_table_mutex);

    // Iterate over the hash table to populate RTT histogram bins
    for (int i = 0; i < TABLE_SIZE; i++) {
        HashNode* node = table->table[i];
        if (node != NULL && node->rtt > 0 && node->rtt < 500000000) {
            int bin_index = node->rtt / BIN_SIZE_RTT;
            if (bin_index < NUM_BINS) {
                rtt_histogram_bins[bin_index]++;
            } else {
                overflow_count++;  // Increment overflow count for RTT
            }
        }
    }

    // Store the overflow count in the last bin
    rtt_histogram_bins[NUM_BINS] = overflow_count;

    // Unlock the hash table after iteration
    // pthread_mutex_unlock(&hash_table_mutex);

    // Store RTT histogram data in a Redis hash
    char key[128];
    snprintf(key, sizeof(key), "histogram:rtt");
    for (int i = 0; i <= NUM_BINS; i++) {  // Include the overflow bin
        redisReply *reply_hset = redisCommand(c, "HSET %s %d %d", key, i, rtt_histogram_bins[i]);
        if (reply_hset == NULL) {
            fprintf(stderr, "Error updating RTT histogram data in Redis: %s\n", c->errstr);
        } else {
            freeReplyObject(reply_hset);
        }
    }
}



// Thread function to find the top 50 flows by accumulated_bytes
void* find_top_flows(void* arg) {
    while (1) {  // Infinite loop

        HashNode* top_flows[TOP_N] = {0};
        int valid_entries = 0;

        // Lock the hash table for safe iteration
        // pthread_mutex_lock(&hash_table_mutex);

        int total_flows = 0;
        long int total_bytes = 0;
        // Iterate over the hash table
        for (int i = 0; i < TABLE_SIZE; i++) {
            HashNode* node = table->table[i];
            if (node != NULL && node->accumulated_bytes > 0) {
                total_flows++;
                total_bytes += node->accumulated_bytes;
            }
        }
        // printf("total_bytes: %ld\n", total_bytes);
        update_total_flows(context, total_flows);
        update_total_flows_bytes(context, total_bytes);
        create_histogram(context);
        create_rtt_histogram(context);

        // Iterate over the hash table
        for (int i = 0; i < TABLE_SIZE; i++) {
            HashNode* node = table->table[i];
            if (node != NULL && node->accumulated_bytes > 0) {
                // Add the valid node to the array
                top_flows[valid_entries++] = node;
                
                // If more than TOP_N flows, we need to expand the array (can be handled with dynamic memory if necessary)
                if (valid_entries >= TOP_N) break;
            }
        }

        // Unlock the hash table after iteration
        // pthread_mutex_unlock(&hash_table_mutex);

        // Sort the array by accumulated_bytes
        qsort(top_flows, valid_entries, sizeof(HashNode*), compare_by_accumulated_bytes);
        maintain_top_flows(context);
        // Push top 50 flows to Redis
        for (int i = 0; i < valid_entries; i++) {
            HashNode* node = top_flows[i];
            push_flow_to_redis(context, node);
        }


        // // Print the top 50 flows
        // printf("Top %d Flows by accumulated_bytes:\n", TOP_N);
        // for (int i = 0; i < valid_entries; i++) {
        //     HashNode* node = top_flows[i];
        //     printf("Flow %d: SrcIP: %u, DstIP: %u, Accumulated Bytes: %lu\n", 
        //         i+1, node->payload.src_ip, node->payload.dst_ip, node->accumulated_bytes);
        // }
        // printf("\n-------------------\n\n");

        // Sleep for a while before repeating (e.g., 5 seconds)
        sleep(1);
    }


    return NULL;
}
