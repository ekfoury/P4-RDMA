
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <ncurses.h>

#include "dict.h"
#include "rdma_payload.h"
#include "constants.h"
#include <string.h>
#include <arpa/inet.h> // For inet_ntoa
#include <inttypes.h>
#include <murmurhash.h>
#include <maxminddb.h>
#include "./libgeohash/geohash.h"  // Include your geohash library header


#include<hiredis/hiredis.h>


FILE *csv_file = NULL;

static int file_index = 0;

FILE* open_new_csv_file() {
    char filename[50];
    snprintf(filename, sizeof(filename), "CSVs/output_%d.csv", file_index++);
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Failed to open CSV file");
        return NULL;
    }
    // Write CSV header
    fprintf(file, "timestamp,src_ip,dst_ip,src_port,dst_port,rtt,bytes,ack_bytes,pkts,loss,end_timestamp,duration\n");
    return file;
}

// Define min function for multiple arguments
unsigned int min(unsigned int a, unsigned int b, unsigned int c, unsigned int d) {
    unsigned int min_val = a;
    if (b < min_val) min_val = b;
    if (c < min_val) min_val = c;
    if (d < min_val) min_val = d;
    return min_val;
}

// Function to open the CSV file
void open_csv_file(const char *filename) {
    if (csv_file == NULL) {
        csv_file = fopen(filename, "w");
        if (csv_file == NULL) {
            perror("Failed to open CSV file");
            return;
        }
        // Write CSV header
        fprintf(csv_file, "src_ip,dst_ip,src_port,dst_port,rtt,bytes,loss\n");
    }
}



unsigned int hash(uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port) {
    unsigned long hash = 5381;
    hash = ((hash << 5) + hash) + src_ip;
    hash = ((hash << 5) + hash) + dst_ip;
    hash = ((hash << 5) + hash) + src_port;
    hash = ((hash << 5) + hash) + dst_port;
    return (unsigned long)(hash % 10485750);
    //return (unsigned int)(hash);
}

HashTable* create_table() {
    HashTable* table = malloc(sizeof(HashTable));
    if (table == NULL) {
        fprintf(stderr, "Failed to allocate memory for hash table\n");
        exit(EXIT_FAILURE);
    }
    printf("sizeof(table->table): %ld\n", sizeof(table->table));
    memset(table->table, 0, sizeof(table->table));
    return table;
}

// Define a buffer size for the Redis command strings
#define COMMAND_BUFFER_SIZE 4096



int write_tput_file(const char *filename, uint64_t timestamp, float tput) {
    FILE *file = fopen(filename, "a");  // Open in text append mode
    if (!file) {
        perror("Error opening file");
        return -1;
    }

    // Write the value as a decimal string with a newline
    if (fprintf(file, "%lu,%f\n", timestamp, tput) < 0) {
        perror("Error writing to file");
        fclose(file);
        return -1;
    }

    fclose(file);
    return 0;
}

int write_IAT_file(const char *filename, uint64_t timestamp, uint32_t value) {
    FILE *file = fopen(filename, "a");  // Open in text append mode
    if (!file) {
        perror("Error opening file");
        return -1;
    }

    // Write the value as a decimal string with a newline
    if (fprintf(file, "%lu,%u\n", timestamp, value) < 0) {
        perror("Error writing to file");
        fclose(file);
        return -1;
    }

    fclose(file);
    return 0;
}

int insert(HashTable* table, struct rdma_payload_t payload, redisContext* context, MMDB_s* mmdb) {
    unsigned int index = hash(payload.src_ip, payload.dst_ip, payload.src_port, payload.dst_port);
    //uint32_t index = hash_function(ip_to_string(payload.src_ip), ip_to_string(payload.dst_ip), payload.src_port, payload.dst_port);
    



    // printf("%d %s %s %d %d\n", index, ip_to_string(payload.src_ip), ip_to_string(payload.dst_ip), htobe16(payload.src_port), htobe16(payload.dst_port));
    // printf("index: %d\n", index);
    //Allocate and initialize a new node if the slot is empty
    if (table->table[index] == NULL) {
        //printf("%d %s %s %d %d\n", index, ip_to_string(payload.src_ip), ip_to_string(payload.dst_ip), payload.src_port, payload.dst_port);

        // printf("NULL\n");
        HashNode* new_node = (HashNode*)malloc(sizeof(HashNode));
        if (new_node == NULL) {
            printf("Failed to allocate memory for hash node\n");
            exit(EXIT_FAILURE);
        }
        // table->count += 1;
        // printf("Table count: %d\n", table->count);
        new_node->payload.src_ip = payload.src_ip;
        new_node->payload.dst_ip = payload.dst_ip;
        new_node->payload.src_port = payload.src_port;
        new_node->payload.dst_port = payload.dst_port;
        //new_node->payload.rtt = payload.rtt;  //>> 16;
        //new_node->payload.rtt2 = payload.rtt2; // >> 16;


        new_node->rtt= htonl(payload.rtt)/1000.0/1000.0;
        // if(new_node->rtt == 0) {
        //     new_node->rtt = htonl(payload.rtt)/1000.0/1000.0;
        // }
        
        

        new_node->start_timestamp = payload.timestamp;

        new_node->payload.bytes_sketch0 = payload.bytes_sketch0;
        new_node->payload.bytes_sketch1 = payload.bytes_sketch1;
        new_node->payload.bytes_sketch2 = payload.bytes_sketch2;
        new_node->payload.bytes_sketch3 = payload.bytes_sketch3;
        
        new_node->payload.iat = payload.iat;
        new_node->payload.packet_type = payload.packet_type;
        new_node->payload.pkt_count_sketch0 = payload.pkt_count_sketch0;
        new_node->payload.pkt_count_sketch1 = payload.pkt_count_sketch1;
        new_node->payload.pkt_count_sketch2 = payload.pkt_count_sketch2;
        new_node->payload.pkt_count_sketch3 = payload.pkt_count_sketch3;

        new_node->payload.ack_no = payload.ack_no;
        new_node->payload.timestamp = payload.timestamp;

        new_node->prev_payload = new_node->payload; 

        uint32_t min_payload = min(htonl(payload.bytes_sketch0), htonl(payload.bytes_sketch1), htonl(payload.bytes_sketch2), htonl(payload.bytes_sketch3));
        uint16_t min_pkts = min(htobe16(payload.pkt_count_sketch0), htobe16(payload.pkt_count_sketch1), htobe16(payload.pkt_count_sketch2), htobe16(payload.pkt_count_sketch3));
        
        new_node->accumulated_bytes = min_payload;
        new_node->accumulated_pkts = min_pkts;
        new_node->bytes = min_payload;
        new_node->pkts = min_pkts;
        new_node->report_count = 0;
        new_node->prev_timestamp_tput = payload.timestamp;
        new_node->prev_ack_no = payload.ack_no;

        table->table[index] = new_node; 
        
        time_t current_time = time(NULL);  // Get the current UNIX timestamp
        
        // REDIS UNCOMMENT BELOW

        // char redis_command_create_accumulated_bytes[COMMAND_BUFFER_SIZE];
        // char redis_command_create_rtt[COMMAND_BUFFER_SIZE];
        // char redis_command_create_loss[COMMAND_BUFFER_SIZE];


        // // Create time series for accumulated_bytes, rtt, and loss (done once)
        // snprintf(redis_command_create_accumulated_bytes, 4096, 
        //     "TS.CREATE rdma_payload:accumulated_bytes:%s:%s:%" PRIu16 ":%" PRIu16 " LABELS type bytes src_ip %s dst_ip %s src_port %" PRIu16 " dst_port %" PRIu16,
        //     ip_to_string(payload.src_ip), ip_to_string(payload.dst_ip),
        //     htobe16(payload.src_port), htobe16(payload.dst_port),
        //     ip_to_string(payload.src_ip), ip_to_string(payload.dst_ip), 
        //     htobe16(payload.src_port), htobe16(payload.dst_port));
        // // printf("Redis: %s\n", redis_command_create_accumulated_bytes);

        // snprintf(redis_command_create_rtt, 4096, 
        //     "TS.CREATE rdma_payload:rtt:%s:%s:%" PRIu16 ":%" PRIu16 " LABELS type rtt src_ip %s dst_ip %s src_port %" PRIu16 " dst_port %" PRIu16,
        //     ip_to_string(payload.src_ip), ip_to_string(payload.dst_ip),
        //     htobe16(payload.src_port), htobe16(payload.dst_port),
        //     ip_to_string(payload.src_ip), ip_to_string(payload.dst_ip), 
        //     htobe16(payload.src_port), htobe16(payload.dst_port));

        // snprintf(redis_command_create_loss, 4096, 
        //     "TS.CREATE rdma_payload:loss:%s:%s:%" PRIu16 ":%" PRIu16 " LABELS type loss src_ip %s dst_ip %s src_port %" PRIu16 " dst_port %" PRIu16,
        //     ip_to_string(payload.src_ip), ip_to_string(payload.dst_ip),
        //     htobe16(payload.src_port), htobe16(payload.dst_port),
        //     ip_to_string(payload.src_ip), ip_to_string(payload.dst_ip), 
        //     htobe16(payload.src_port), htobe16(payload.dst_port));
        
        // redisAppendCommand(context, redis_command_create_accumulated_bytes);
        // redisReply *reply;
        // redisGetReply(context, (void **)&reply);
        // redisAppendCommand(context, redis_command_create_rtt);
        // redisGetReply(context, (void **)&reply);
        // redisAppendCommand(context, redis_command_create_loss);
        // redisGetReply(context, (void **)&reply);
       
        
        // int gai_error, mmdb_error;
        // MMDB_lookup_result_s result = MMDB_lookup_string(mmdb, ip_to_string(payload.src_ip), &gai_error, &mmdb_error);

        // double latitude = 0.0;
        // double longitude = 0.0;

        // if (result.found_entry) {
        //     MMDB_entry_data_s entry_data;

        //     // Look up the latitude
        //     int lookup_status_lat = MMDB_get_value(&result.entry, &entry_data, "location", "latitude", NULL);
        //     if (lookup_status_lat == MMDB_SUCCESS && entry_data.has_data && entry_data.type == MMDB_DATA_TYPE_DOUBLE) {
        //         latitude = entry_data.double_value;
        //     } 

        //     // Look up the longitude
        //     int lookup_status_lon = MMDB_get_value(&result.entry, &entry_data, "location", "longitude", NULL);
        //     if (lookup_status_lon == MMDB_SUCCESS && entry_data.has_data && entry_data.type == MMDB_DATA_TYPE_DOUBLE) {
        //         longitude = entry_data.double_value;
        //     } 
        // } 

        // char redis_command_location[COMMAND_BUFFER_SIZE];

        // if(longitude != 0 && latitude != 0) {
        //     char* geo_hash;  // Buffer to store geohash string
        //     geo_hash = geohash_encode(latitude, longitude, 11);
        //     snprintf(redis_command_location, 4096, 
        //     "HSET geohash %s %s", ip_to_string(payload.src_ip), geo_hash);
        //     redisAppendCommand(context, redis_command_location);            
        //     redisReply *reply;
        //     redisGetReply(context, (void **)&reply);
        // }

        // REDIS UNCOMMENT BELOW END

        return 1;

    } else if(htobe64(payload.timestamp) > htobe64(table->table[index]->payload.timestamp)) {

        HashNode* node = table->table[index];
        // uint32_t rtt = node->payload.rtt;
        //node->payload = payload; 
        // node->payload.rtt = rtt;
        
        node->payload.bytes_sketch0 = payload.bytes_sketch0;
        node->payload.bytes_sketch1 = payload.bytes_sketch1;
        node->payload.bytes_sketch2 = payload.bytes_sketch2;
        node->payload.bytes_sketch3 = payload.bytes_sketch3;
        

        node->payload.iat = payload.iat;
        // printf("IAT: %d\n", htonl(payload.iat));
        //if(htobe16(payload.dst_port) == 5201 && htobe16(payload.src_port) == 5555) 
        write_IAT_file("iat.csv", htobe64(payload.timestamp), htonl(payload.iat));
        node->payload.packet_type = payload.packet_type;
        node->payload.pkt_count_sketch0 = payload.pkt_count_sketch0;
        node->payload.pkt_count_sketch1 = payload.pkt_count_sketch1;
        node->payload.pkt_count_sketch2 = payload.pkt_count_sketch2;
        node->payload.pkt_count_sketch3 = payload.pkt_count_sketch3;

        node->payload.ack_no = payload.ack_no;

        node->payload.timestamp = payload.timestamp;

        //printf("payload.ack_no %u\n", htonl(payload.ack_no));


        // Calculate the minimum values
        uint32_t min_payload = min(htonl(payload.bytes_sketch0), htonl(payload.bytes_sketch1), htonl(payload.bytes_sketch2), htonl(payload.bytes_sketch3));
        uint32_t min_prev_payload = min(htonl(node->prev_payload.bytes_sketch0), htonl(node->prev_payload.bytes_sketch1), htonl(node->prev_payload.bytes_sketch2), htonl(node->prev_payload.bytes_sketch3));
        uint32_t current_diff = (min_payload >= node->bytes) ? (min_payload - node->bytes) : 0;

        uint16_t min_pkts = min(htobe16(payload.pkt_count_sketch0), htobe16(payload.pkt_count_sketch1), htobe16(payload.pkt_count_sketch2), htobe16(payload.pkt_count_sketch3));
        //printf("min packets %u\n", min_pkts);
        uint16_t current_diff_pkts = (min_pkts >= node->pkts) ? (min_pkts - node->pkts) : 0;

        // Check for reset
        // if (min_payload < min_prev_payload) {
            // Reset detected
            // Add the difference since last update to accumulated_bytes
            node->accumulated_bytes += current_diff;
            node->accumulated_pkts += current_diff_pkts + 1; // ACK return is counted
            // Reset node->bytes to the new minimum value
            node->bytes = min_payload;
            node->pkts = min_pkts;
        // } else {
            // No reset
            // Add the current difference to accumulated_bytes
            // node->accumulated_bytes += current_diff;
            // Update node->bytes to the new value
            // node->bytes = min_payload;
        // }

        node->sent_interval += current_diff_pkts + 1;  // Track interval-specific packets




        //   if(min(htonl(payload.ack_count_sketch0), htonl(payload.ack_count_sketch1), htonl(payload.ack_count_sketch2), htonl(payload.ack_count_sketch3)) > 1) {
        //     node->loss += 1;
        //   }
         

        
        //printf("payload.ack_no: %d\n", htonl(payload.ack_no));
        //printf("node->prev_ack_no: %d\n", htonl(node->prev_ack_no));




        

        if(htonl(payload.packet_type) == 4) { //fin / rst packet
            node->end_timestamp = payload.timestamp;
            node->duration = htobe64(node->end_timestamp) - htobe64(node->start_timestamp);
        }

       
        if(htonl(payload.ack_no) == htonl(node->prev_payload.ack_no)) { //} && htonl(payload.ack_no) != htonl(node->retr_ack_no)) {
            node->retr++;
            //node->retr_ack_no = payload.ack_no;
        } 

        // uint32_t unack;
        // if(htonl(payload.seq_no) > htonl(payload.ack_no)) {
        //     //printf("Seq No - Ack No: %u\n", htonl(payload.seq_no) - htonl(payload.ack_no));
        //     unack = htonl(payload.seq_no) - htonl(payload.ack_no);
        //     //uint32_t ack_no = ntohl(payload.ack_no) - unack;        
        // }
        // uint32_t diff = htonl(payload.ack_no) - htonl(node->prev_payload.ack_no);             
        // if(diff > 0) {
        //     node->acknowledged_bytes += diff;
        //     printf("Seq No: %u, ACK_NO: %u\n", htonl(payload.seq_no), htonl(payload.ack_no));
        //     printf("node->accumulated_bytes: %f, node->acknowledged_bytes: %f\n", (float)node->accumulated_bytes  - (float)unack, (float)node->acknowledged_bytes);
        //     float loss_rate = 0.0;
        //     loss_rate = 1.0 - ((float)node->acknowledged_bytes / ((float)node->accumulated_bytes - (float)unack));
        //     printf("Packet Loss Rate: %f%%\n", loss_rate * 100);
        // } 



        

        //if (bytes_sent_in_interval > 0) { // Ensure no division by zero
            
            //loss_rate -=0.04;
            //if(loss_rate <0 || loss_rate > 0.5) 
             // loss_rate = 0;
        //}
        //printf("Packet Loss Rate: %f%%\n", loss_rate * 100);

        //printf("SEQ No: %u, ACK No: %u\n", htonl(payload.dummy), htonl(payload.ack_no));



        float time_diff = ((float)htobe64(payload.timestamp) - (float)htobe64(node->prev_timestamp_tput))/1e9;
        if(time_diff >= 1.0) {
            //printf("node->accumulated_bytes: %f, node->acknowledged_bytes: %f\n", (float)node->accumulated_bytes, (float)node->acknowledged_bytes);

            node->throughput = (((float)node->accumulated_bytes - (float)node->prev_accumulated_bytes) * 8) / time_diff ;

            node->prev_timestamp_tput = payload.timestamp;
            node->prev_accumulated_bytes = node->accumulated_bytes;
            node->prev_acknowledged_bytes = node->acknowledged_bytes;
             node->loss = (float)node->retr / (float)node->sent_interval;
            //printf("Total retr: %u\n", node->retr);
            //printf("Packet Loss Rate: %f%%\n", node->loss * 100);
            node->retr = 0;
            node->sent_interval = 0;  // Reset interval packet count

            if(htobe16(payload.dst_port) == 5201 && htobe16(payload.src_port) == 5555) 
                write_tput_file("tput.csv", htobe64(payload.timestamp), node->throughput);
            //printf("Throughput: %f bps\n", node->throughput);

        }
        


        //  if(htonl(payload.ack_no) > htonl(node->prev_payload.ack_no)) 
        //     node->acknowledged_bytes += htonl(payload.ack_no) - htonl(node->prev_payload.ack_no); 

        // if(htobe16(payload.dst_port) == 5201) // && htobe16(payload.src_port) == 5555) 
        //     //printf_entry(payload, node->accumulated_bytes ,htonl(payload.ack_no), htonl(node->prev_payload.ack_no), htonl(payload.ack_no) - htonl(node->prev_payload.ack_no), node->acknowledged_bytes, node->loss);

        //     //htobe64(payload.timestamp) > htobe64(table->table[index]->payload.timestamp
        //     printf("%lu   %lu", htobe64(payload.timestamp), htobe64(table->table[index]->payload.timestamp));

        // //   }
          node->prev_payload = node->payload;


        // REDIS UNCOMMENT BELOW

        // if(node->report_count % 100 == 0) {
        //     char redis_command_add_accumulated_bytes[COMMAND_BUFFER_SIZE];
        //     char redis_command_add_rtt[COMMAND_BUFFER_SIZE];
        //     char redis_command_add_loss[COMMAND_BUFFER_SIZE];


        //     // Add data to the time series for accumulated_bytes
        //     snprintf(redis_command_add_accumulated_bytes, COMMAND_BUFFER_SIZE, 
        //         "TS.ADD rdma_payload:accumulated_bytes:%s:%s:%" PRIu16 ":%" PRIu16 " * %f",
        //         ip_to_string(payload.src_ip), ip_to_string(payload.dst_ip),
        //         htobe16(payload.src_port), htobe16(payload.dst_port),
        //         (float)table->table[index]->accumulated_bytes);

        //     // Add data to the time series for rtt
        //     snprintf(redis_command_add_rtt, COMMAND_BUFFER_SIZE, 
        //         "TS.ADD rdma_payload:rtt:%s:%s:%" PRIu16 ":%" PRIu16 " * %" PRIu32,
        //         ip_to_string(payload.src_ip), ip_to_string(payload.dst_ip),
        //         htobe16(payload.src_port), htobe16(payload.dst_port),
        //         (payload.rtt >> 16));

        //     // Add data to the time series for loss
        //     snprintf(redis_command_add_loss, COMMAND_BUFFER_SIZE, 
        //         "TS.ADD rdma_payload:loss:%s:%s:%" PRIu16 ":%" PRIu16 " * %.4f",
        //         ip_to_string(payload.src_ip), ip_to_string(payload.dst_ip),
        //         htobe16(payload.src_port), htobe16(payload.dst_port),
        //         table->table[index]->loss);
            

            
        //     // printf("Redis add: %s\n", redis_command_add_loss);

        //     redisAppendCommand(context, redis_command_add_accumulated_bytes);
        //     // redisReply *reply;
        //     // redisGetReply(context, (void **)&reply);
        //     redisAppendCommand(context, redis_command_add_rtt);
        //     // redisGetReply(context, (void **)&reply);
        //     redisAppendCommand(context, redis_command_add_loss);
        //     // redisGetReply(context, (void **)&reply);
        // }
        // // REDIS UNCOMMENT BELOW END

        node->report_count+= 1;

        return 1;
    } 
    
    return 0; 

}

struct rdma_payload_t* search(HashTable* table, uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port) {
    unsigned int index = hash(src_ip, dst_ip, src_port, dst_port);
    HashNode* current = table->table[index];
    if (current != NULL) {
        if (current->payload.src_ip == src_ip && current->payload.dst_ip == dst_ip &&
            current->payload.src_port == src_port && current->payload.dst_port == dst_port) {
            return &current->payload;
        }
    }
    return NULL;
}

void free_table(HashTable* table) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        HashNode* current = table->table[i];
        if (current != NULL) {
            HashNode* temp = current;
            free(temp);
        }
    }
    free(table);
}

char* ip_to_string(unsigned int ip)
{
    char* buffer = (char*)malloc(16 * sizeof(char));
    if (buffer == NULL) {
        return NULL;
    }

    // Convert IP from network byte order to host byte order
    unsigned int network_ip = ntohl(ip);

    // Convert the decimal IP to its string representation
    snprintf(buffer, 16, "%u.%u.%u.%u",
             (network_ip >> 24) & 0xFF,
             (network_ip >> 16) & 0xFF,
             (network_ip >> 8) & 0xFF,
             network_ip & 0xFF);
    return buffer;
}



void printf_entry( struct rdma_payload_t payload, uint32_t bytes, uint32_t ackno, uint32_t ackno2, uint32_t diff, uint32_t ack_bytes, float loss) {
    printf("%-15s %-15s %-8d %-8d %-10u %-10u %-10u %-10u %-10u %f\n",
           ip_to_string(payload.src_ip),
           ip_to_string(payload.dst_ip),
           htobe16(payload.src_port),
           htobe16(payload.dst_port),
        //    htonl(payload.rtt),
        //    htonl(payload.rtt2),
        //    htonl(payload.ack_no),
           bytes,
           ackno,
           ackno2,
           diff,
           ack_bytes,
           loss
           );
}

// void printf_table(HashTable* table) {
//     for (int i = 0; i < TABLE_SIZE; i++) {
//         HashNode* current = table->table[i];
//         if  (current != NULL) {
//             printf_entry(current->payload, current->bytes, current->acknowledged_bytes);
//         }
//     }
// }

void print_table_header(WINDOW *win) {
    // Header with fixed column widths
    mvwprintw(win, 0, 0, "%-15s %-15s %-8s %-8s %-15s %-15s %-15s %-10s",
              "Src IP", "Dst IP", "Src Port", "Dst Port", "RTT", "Bytes", "Retr", "Count");


}

void print_entry(WINDOW *win, int row, struct rdma_payload_t payload, uint32_t bytes, uint32_t loss) { 
      uint32_t rtt = htonl(payload.rtt);
      mvwprintw(win, row, 0, "%-15s %-15s %-8d %-8d %-6u %-10f %-10f",
           ip_to_string(payload.src_ip),
           ip_to_string(payload.dst_ip),
           htobe16(payload.src_port),
           htobe16(payload.dst_port),
           rtt,
           (float)(bytes),
        //    (float)(bytes * 32.0),
           (float)(loss*1.0));
    
    // fprintf(csv_file, "%s,%s,%u,%u,%u,%f,%f\n",
    //     ip_to_string(payload.src_ip),
    //     ip_to_string(payload.dst_ip),
    //     htobe16(payload.src_port),
    //     htobe16(payload.dst_port),
    //     rtt,
    //     (float)(bytes * 32.0),
    //     (float)(loss));

    // // Flush the buffer periodically to ensure data is written
    // static int counter = 0;
    // if (++counter % 100 == 0) {  // Adjust the frequency as needed
    //     fflush(csv_file);
    // }

}

void close_csv_file() {
    if (csv_file != NULL) {
        fclose(csv_file);
        csv_file = NULL;
    }
}


void print_table(WINDOW *win, HashTable* table, int count) {
    // Close the previous file if it's open
    static FILE *csv_file = NULL;
    if (csv_file != NULL) {
        fclose(csv_file);
    }
    
    // Open a new CSV file
    csv_file = open_new_csv_file();
    if (csv_file == NULL) {
        return;
    }

    // wclear(win);
    // box(win, 0, 0);
    // print_table_header(win);
    int row = 1;
    for (int i = 0; i < TABLE_SIZE; i++) {
        HashNode* current = table->table[i];
        if  (current != NULL) {
            
            fprintf(csv_file, "%lu,%s,%s,%u,%u,%f,%f,%f,%f,%f,%lu,%lu\n",
                htobe64(current->start_timestamp),
                //be64toh(current->start_timestamp),
                ip_to_string(current->payload.src_ip),
                ip_to_string(current->payload.dst_ip),
                htobe16(current->payload.src_port),
                htobe16(current->payload.dst_port),
                current->rtt,
                // (float)(current->accumulated_bytes * 32.0),
                (float)(current->accumulated_bytes),
                (float)(current->acknowledged_bytes),
                (float)(current->accumulated_pkts),

                (float) current->loss,
                htobe64(current->end_timestamp),
                (current->duration));
            //fflush(csv_file);  // Ensure data is written to the file immediately

            row++;
        }
    }
    wrefresh(win);
}

