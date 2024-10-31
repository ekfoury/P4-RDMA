#ifndef RDMA_PAYLOAD_H
#define RDMA_PAYLOAD_H

#include <stdint.h>

struct rdma_payload_t {
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t rtt;
    uint32_t bytes_sketch0;
    uint32_t bytes_sketch1;
    uint32_t bytes_sketch2;
    uint32_t bytes_sketch3;
    uint32_t iat;
    uint32_t packet_type;
    uint16_t pkt_count_sketch0;
    uint16_t pkt_count_sketch1;
    uint16_t pkt_count_sketch2;
    uint16_t pkt_count_sketch3;
    uint32_t ack_no;
    uint32_t seq_no;
    uint64_t timestamp;
};

#endif // RDMA_PAYLOAD_H
