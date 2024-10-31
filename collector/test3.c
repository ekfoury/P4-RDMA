#include <stdio.h>
#include <stdint.h>

#define TABLE_SIZE 1000  // Example table size

// Simple hash function (e.g., DJB2 hash function)
unsigned int hash_function(const char *str) {
    unsigned long hash = 5381;
    int c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }

    return hash;
}

// Function to get hash value within the range [0, TABLE_SIZE-1]
unsigned int get_table_index(const char *str) {
    unsigned int hash = hash_function(str);
    return hash % TABLE_SIZE;
}

int main() {
    printf("%ld\n", 8 % 100000); 
    return 0;
}
