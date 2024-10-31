#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "rdma_cm_manager.h"
#include "influxdb_helper.h"
#include "rdma_payload.h"
#include "constants.h"
#include "dict.h"
#include <ncurses.h>
#include <unistd.h>  
#include <curl/curl.h>

int main(int argc, char *argv[]) {
    // Initialize ncurses
    initscr();
    noecho();
    cbreak();
    curs_set(FALSE);

    struct rdma_payload_t       *buf;
    int err;

    HashTable* table = create_table();
    
    // Create a window for the table
    int win_height = 50; // Adjust as needed
    int win_width = 160; // Adjust as needed
    WINDOW *win = newwin(win_height, win_width, 0, 0);

    err = initialize_rdmacm(&buf);
    if(err)
        return 1;
    
    CURL *curl = initialize_curl();
    if (!curl) {
        fprintf(stderr, "Failed to initialize CURL\n");
        return 1;
    }

    clock_t period_start_time = clock();
    int count = 0;
    int prev_count = 0;
    for(int i = 0; i <= CIRCULAR_BUFFER_SIZE; i++) {
        if (i == CIRCULAR_BUFFER_SIZE) {
            i = 0;
            clock_t period_end_time = clock();
            double period_elapsed_time = (double)(period_end_time - period_start_time) / CLOCKS_PER_SEC;
            print_table(win, table, count);
            //printf_table(table);
            period_start_time = clock();
            prev_count = count;
            count = 0;
            sleep(1);
        }
        if(htonl(buf[i].rtt) != 0) { // && htonl(buf[i].rtt) < 100000000) {
            count+=1;
            unsigned int index = hash(buf[i].src_ip, buf[i].dst_ip, buf[i].src_port, buf[i].dst_port);
            //if(table->table[index] == NULL) {
            //    write_data(curl, &buf[i]);  // Write each valid entry to InfluxDB
            //}
            insert(table, buf[i]);
        }
    }
    return 0;
}
