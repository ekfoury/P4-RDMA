
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
    fprintf(file, "src_ip,dst_ip,src_port,dst_port,rtt,bytes,loss\n");
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

char* insert(HashTable* table, struct rdma_payload_t payload) {
    unsigned int index = hash(payload.src_ip, payload.dst_ip, payload.src_port, payload.dst_port);
    //uint32_t index = hash_function(ip_to_string(payload.src_ip), ip_to_string(payload.dst_ip), payload.src_port, payload.dst_port);
    
    // PT98RMQaINAKfSisUaMCV8jExswMyYMrSLj7Ci2KP6kCWDMXRJFWdcP-K39r7f1jZxH5V8MStDBJbZCPz5T6Cg==

    
        // Allocate memory for the InfluxDB line protocol string
    char* influxdb_line = (char*)malloc(4096);
    if (influxdb_line == NULL) {
        printf("Failed to allocate memory for InfluxDB line protocol string\n");
        exit(EXIT_FAILURE);
    }


    // printf("%d %s %s %d %d\n", index, ip_to_string(payload.src_ip), ip_to_string(payload.dst_ip), htobe16(payload.src_port), htobe16(payload.dst_port));
    // printf("index: %d\n", index);
    //Allocate and initialize a new node if the slot is empty
    if (table->table[index] == NULL) {
        //printf("%d %s %s %d %d\n", index, ip_to_string(payload.src_ip), ip_to_string(payload.dst_ip), payload.src_port, payload.dst_port);

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
        new_node->payload.rtt = payload.rtt >> 16;
        new_node->payload.rtt2 = payload.rtt2 >> 16;

        new_node->payload.bytes_sketch0 = payload.bytes_sketch0;
        new_node->payload.bytes_sketch1 = payload.bytes_sketch1;
        new_node->payload.bytes_sketch2 = payload.bytes_sketch2;
        new_node->payload.bytes_sketch3 = payload.bytes_sketch3;
        
        new_node->payload.ack_count_sketch0 = payload.ack_count_sketch0;
        new_node->payload.ack_count_sketch1 = payload.ack_count_sketch1;
        new_node->payload.ack_count_sketch2 = payload.ack_count_sketch2;
        new_node->payload.ack_count_sketch3 = payload.ack_count_sketch3;

        new_node->payload.ack_no = payload.ack_no;

        new_node->payload.timestamp = payload.timestamp;

        new_node->prev_payload = new_node->payload; 

        uint32_t min_payload = min(htonl(payload.bytes_sketch0), htonl(payload.bytes_sketch1), htonl(payload.bytes_sketch2), htonl(payload.bytes_sketch3));
        
        new_node->accumulated_bytes = min_payload;
        new_node->bytes = min_payload;

        table->table[index] = new_node;        

    } else if(htobe64(payload.timestamp) > htobe64(table->table[index]->payload.timestamp)) {

        HashNode* node = table->table[index];
        node->payload = payload; 

        // Calculate the minimum values
        uint32_t min_payload = min(htonl(payload.bytes_sketch0), htonl(payload.bytes_sketch1), htonl(payload.bytes_sketch2), htonl(payload.bytes_sketch3));
        uint32_t min_prev_payload = min(htonl(node->prev_payload.bytes_sketch0), htonl(node->prev_payload.bytes_sketch1), htonl(node->prev_payload.bytes_sketch2), htonl(node->prev_payload.bytes_sketch3));


        uint32_t current_diff = (min_payload >= node->bytes) ? (min_payload - node->bytes) : 0;

        // Check for reset
        // if (min_payload < min_prev_payload) {
            // Reset detected
            // Add the difference since last update to accumulated_bytes
            node->accumulated_bytes += current_diff;
            // Reset node->bytes to the new minimum value
            node->bytes = min_payload;
        // } else {
            // No reset
            // Add the current difference to accumulated_bytes
            // node->accumulated_bytes += current_diff;
            // Update node->bytes to the new value
            // node->bytes = min_payload;
        // }




        //   if(min(htonl(payload.ack_count_sketch0), htonl(payload.ack_count_sketch1), htonl(payload.ack_count_sketch2), htonl(payload.ack_count_sketch3)) > 1) {
        //     node->loss += 1;
        //   }
         
         if(node->prev_payload.ack_no != 0) {
            int diff = htonl(payload.ack_no) - htonl(node->prev_payload.ack_no); 
            if(diff > 0) {
                node->acknowledged_bytes += diff;
                if(node->accumulated_bytes > 0) {
                    node->loss = 1 - ((float)node->acknowledged_bytes / (float)node->accumulated_bytes);
                    node->loss = node->loss > 0? node->loss:0;
                } else {
                    node->loss = 0;
                }
            }
         }


        //  if(htonl(payload.ack_no) > htonl(node->prev_payload.ack_no)) 
        //     node->acknowledged_bytes += htonl(payload.ack_no) - htonl(node->prev_payload.ack_no); 

        // if(htobe16(payload.dst_port) == 5201) // && htobe16(payload.src_port) == 5555) 
        //     //printf_entry(payload, node->accumulated_bytes ,htonl(payload.ack_no), htonl(node->prev_payload.ack_no), htonl(payload.ack_no) - htonl(node->prev_payload.ack_no), node->acknowledged_bytes, node->loss);

        //     //htobe64(payload.timestamp) > htobe64(table->table[index]->payload.timestamp
        //     printf("%lu   %lu", htobe64(payload.timestamp), htobe64(table->table[index]->payload.timestamp));

        // //   }
          node->prev_payload = node->payload;

        } else {
            return "";
        }

            // Format the InfluxDB line protocol string with only the required fields
        snprintf(influxdb_line, 4096, 
            "rdma_payload,src_ip=%s,dst_ip=%s,src_port=%" PRIu16 ",dst_port=%" PRIu16 
            " accumulated_bytes=%f,rtt=%" PRIu32 ",loss=%.4f\n",
            ip_to_string(payload.src_ip), ip_to_string(payload.dst_ip), htobe16(payload.src_port), htobe16(payload.dst_port),
            (float)table->table[index]->accumulated_bytes, (payload.rtt >> 16), table->table[index]->loss);

        return influxdb_line;


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
      if(rtt == 0) {
        rtt = htonl(payload.rtt2);
      }
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

    wclear(win);
    box(win, 0, 0);
    print_table_header(win);
    int row = 1;
    for (int i = 0; i < TABLE_SIZE; i++) {
        HashNode* current = table->table[i];
        if  (current != NULL) {
            print_entry(win, row, current->payload, current->accumulated_bytes, current->acknowledged_bytes);
            //current = current->next;
                        // Write to CSV file
            uint32_t rtt = htonl(current->payload.rtt);
            if(rtt == 0) {
                rtt = htonl(current->payload.rtt2);
            }

            fprintf(csv_file, "%s,%s,%u,%u,%u,%f,%f\n",
                ip_to_string(current->payload.src_ip),
                ip_to_string(current->payload.dst_ip),
                htobe16(current->payload.src_port),
                htobe16(current->payload.dst_port),
                rtt,
                // (float)(current->accumulated_bytes * 32.0),
                (float)(current->accumulated_bytes),
                (float)(current->acknowledged_bytes));
            //fflush(csv_file);  // Ensure data is written to the file immediately

            row++;
        }
    }
    wrefresh(win);
}

