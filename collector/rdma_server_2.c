#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "rdma_cm_manager.h"
#include "rdma_payload.h"
#include "constants.h"
#include "dict.h"
#include <ncurses.h>
#include <unistd.h>  

int main(int argc, char *argv[]) {
    // Initialize ncurses
    initscr();
    noecho();
    cbreak();
    curs_set(FALSE);

    struct rdma_payload_t       *buf;
    int err;

    HashTable* table = create_table();
    
    //struct rdma_payload_t payload1 = {0x0A000001, 0x0A000002, 1234, 5678, 100, 1000, 10, 10000, 1609459200};
    //struct rdma_payload_t payload2 = {0x0A000003, 0x0A000004, 1235, 5679, 200, 2000, 20, 20000, 1609459201};

    //struct rdma_payload_t* result = search(table, 0x0A000001, 0x0A000002, 1234, 5678);
    /*
    if (result != NULL) {
        print_entry(*result);
    } else {
        printf("Payload not found\n");
    }
    */

    // Create a window for the table
    int win_height = 50; // Adjust as needed
    int win_width = 160; // Adjust as needed
    WINDOW *win = newwin(win_height, win_width, 0, 0);


    err = initialize_rdmacm(&buf);
    if(err)
        return 1;
    

    clock_t period_start_time = clock();
    int count = 0;
    int prev_count = 0;
    for(int i = 0; i <= CIRCULAR_BUFFER_SIZE; i++) {
        if (i == CIRCULAR_BUFFER_SIZE) {
            i = 0;
            clock_t period_end_time = clock();
            double period_elapsed_time = (double)(period_end_time - period_start_time) / CLOCKS_PER_SEC;
            //clear_console();
            // Print the hash table
            print_table(win, table);

            //if(prev_count != count)
            //printf("Count: %d, elapsed time for this period: %.6f seconds\n", count, period_elapsed_time);
            period_start_time = clock();
            prev_count = count;
            count = 0;
            sleep(1);
        }
        if(buf[i].rtt != 0) {
            count+=1;
            //struct rdma_payload_t* result = search(table, buf[i].src_ip, buf[i].dst_ip, buf[i].src_port, buf[i].dst_port);
            insert(table, buf[i]);
            //print_table(table);
        }
    }



    return 0;
}
