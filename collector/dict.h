
#ifndef DICT_H
#define DICT_H

#include "./rdma_payload.h"
#include "./constants.h"
#include <ncurses.h>
#include <pthread.h>
#include <hiredis/hiredis.h>
#include <maxminddb.h>

typedef struct HashNode {
    struct rdma_payload_t prev_payload;
    struct rdma_payload_t payload;
    uint64_t accumulated_bytes; // total accumulated bytes
    uint64_t accumulated_pkts; // total accumulated bytes
    uint32_t bytes;
    uint16_t pkts;
    uint32_t retr;
    uint32_t sent_interval;
    float rtt;
    uint64_t acknowledged_bytes;
    uint64_t prev_accumulated_bytes;
    uint64_t prev_acknowledged_bytes;

    uint64_t prev_timestamp_tput;
    uint32_t prev_ack_no;
    uint32_t retr_ack_no;

    uint64_t start_timestamp;
    uint64_t end_timestamp;
    uint64_t duration;
    float throughput;
    float loss;
    bool changed;
    int report_count; 
} HashNode;

typedef struct {
    HashNode* table[TABLE_SIZE];
    int count;
} HashTable;

unsigned int hash(uint32_t, uint32_t, uint16_t, uint16_t);

HashTable* create_table();

int insert(HashTable*, struct rdma_payload_t, redisContext* c, MMDB_s* mmdb);

struct rdma_payload_t* search(HashTable*, uint32_t, uint32_t, uint16_t, uint16_t);

void free_table(HashTable*);

void print_entry(WINDOW *win, int row, struct rdma_payload_t payload, uint32_t bytes, uint32_t loss);
void print_table(WINDOW *win, HashTable* table, int count);


void printf_table(HashTable* table);
void printf_entry( struct rdma_payload_t payload, uint32_t bytes, uint32_t ackno, uint32_t ackno2, uint32_t diff, uint32_t ack_bytes, float loss);

char* ip_to_string(unsigned int ip);

void open_csv_file(const char *filename);
void close_csv_file();

// Global file pointer
extern FILE *csv_file;

#endif // DICT_H
