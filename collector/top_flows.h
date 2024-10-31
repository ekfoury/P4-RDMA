
#ifndef TOP_FLOWS_H
#define TOP_FLOWS_H

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include "dict.h"


// External declaration of hash_table and mutex for use in multiple files
extern HashTable* table;
extern redisContext* context;
extern pthread_mutex_t hash_table_mutex;
extern MMDB_s mmdb;

// Function declaration to find the top flows
void* find_top_flows(void* arg);

#endif // TOP_FLOWS_H
