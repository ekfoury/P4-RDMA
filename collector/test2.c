#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "cuckoohash_map.hh"

// Define the type of keys and values
typedef std::string KeyType;
typedef int ValueType;

// Create a cuckoo hash map with string keys and int values
typedef cuckoohash_map<KeyType, ValueType> HashTable;

// Initialize the hash table
HashTable* hash_table;
pthread_mutex_t hash_table_mutex = PTHREAD_MUTEX_INITIALIZER;

// Function to add entries to the hash table
void* add_entries(void* arg) {
    int thread_id = *((int*)arg);
    for (int i = 0; i < 1000; ++i) {
        KeyType key = "key" + std::to_string(thread_id * 1000 + i);
        ValueType value = thread_id * 1000 + i;

        pthread_mutex_lock(&hash_table_mutex);
        hash_table->insert(std::make_pair(key, value));
        pthread_mutex_unlock(&hash_table_mutex);
    }
    return NULL;
}

// Function to retrieve entries from the hash table
void* retrieve_entries(void* arg) {
    int thread_id = *((int*)arg);
    for (int i = 0; i < 1000; ++i) {
        KeyType key = "key" + std::to_string(thread_id * 1000 + i);
        ValueType value;

        pthread_mutex_lock(&hash_table_mutex);
        if (hash_table->find(key, value)) {
            printf("Thread %d found %s: %d\n", thread_id, key.c_str(), value);
        }
        pthread_mutex_unlock(&hash_table_mutex);
    }
    return NULL;
}

int main() {
    // Initialize the hash table
    hash_table = new HashTable();

    // Create threads
    const int num_threads = 4;
    pthread_t threads[num_threads];
    int thread_ids[num_threads];

    // Create threads to add entries
    for (int i = 0; i < num_threads; ++i) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, add_entries, &thread_ids[i]);
    }
    // Wait for threads to complete
    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], NULL);
    }

    // Create threads to retrieve entries
    for (int i = 0; i < num_threads; ++i) {
        pthread_create(&threads[i], NULL, retrieve_entries, &thread_ids[i]);
    }
    // Wait for threads to complete
    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], NULL);
    }

    // Clean up
    delete hash_table;
    return 0;
}

