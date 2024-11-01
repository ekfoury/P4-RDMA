#ifndef _CONSTANTS_
#define _CONSTANTS_

//#define CIRCULAR_BUFFER_SIZE 1048576 //2^20
#define CIRCULAR_BUFFER_SIZE 33554432 //16777216 //1048575 //65536 //2^20
#define TABLE_SIZE 33554432 //10485750
#define RDMA_PAYLOAD_SIZE 64

enum { 
    RESOLVE_TIMEOUT_MS = 5000,
};
#endif