/*************************************************************************
 ************* C O N S T A N T S    A N D   T Y P E S  *******************
**************************************************************************/

const bit<16> TYPE_IPV4 = 0x800;
const bit<16> ETHERTYPE_TPID = 0x8100;
const bit<16> ETHERTYPE_IPV4 = 0x0800;

const bit<9> COLLECTOR_PORT = 160;
const bit<9> R6_PORT = 168;
const bit<9> CPU_PORT = 192;
#define COLLECTOR_MAC 0x58a2e117304d

typedef bit<9>  egressSpec_t;
typedef bit<48> macAddr_t;
typedef bit<32> ip4Addr_t;
typedef bit<16> hash_size_t;
typedef bit<8>  pkt_type_t;
const pkt_type_t PKT_TYPE_NORMAL = 1;
const pkt_type_t PKT_TYPE_MIRROR = 2;
typedef bit<3> mirror_type_t;
const mirror_type_t MIRROR_TYPE_I2E = 1;
const mirror_type_t MIRROR_TYPE_E2E = 2;

// Packet-related constants
#define SEQ_PACKET meta.pkt_type==PKT_TYPE_SEQ
#define ACK_PACKET meta.pkt_type==PKT_TYPE_ACK
#define SYNACK_PACKET meta.pkt_type==PKT_TYPE_SYNACK
#define SYN_PACKET meta.pkt_type==PKT_TYPE_SYN
#define FIN_RST_PACKET meta.pkt_type==PKT_TYPE_FIN_RST

#define TCP_PACKET hdr.tcp.isValid()

#define DIR_INITIATOR 0
#define DIR_RESPONDER 1

// Sketch-related constants
#define SKETCH_BUCKET_LENGTH 65535  
#define SKETCH_CELL_BIT_WIDTH 32
#define SKETCH_CELL_BIT_WIDTH_PACKETS 16
#define THRESH 500 

// RDMA-related constants
#define RDMA_ACK hdr.rdma_aeth.isValid()
#define RDMA_PAYLOAD_SIZE 64
#define UDP_LENGTH (40 + RDMA_PAYLOAD_SIZE)
#define IP_LENGTH (60 + RDMA_PAYLOAD_SIZE)
#define RDMA_PACKET_MAC 0x000200000300
#define RDMA_PACKET_DST_PORT 4791
#define RDMA_PACKET_SRC_PORT 63491
#define RDMA_WRITE_OPCODE 10
#define RDMA_WRITE_OPCODE_UC 42
#define RDMA_PACKET_SRC_IP 0xc0a83202


// TM-related constants
#define FROM_CPU ig_intr_md.ingress_port == CPU_PORT
#define FROM_COLLECTOR ig_intr_md.ingress_port == COLLECTOR_PORT
#define FROM_R6 ig_intr_md.ingress_port == R6_PORT
#define SEND_TO(output_port) ig_tm_md.ucast_egress_port=##output_port;


/*************************************************************************
*********************** H E A D E R S  ***********************************
*************************************************************************/

header ethernet_t {
    macAddr_t dstAddr;
    macAddr_t srcAddr;
    bit<16>   ether_type;
}

header ipv4_t {
    bit<4> version;
    bit<4> ihl;
    bit<8> tos;
    bit<16> total_len;
    bit<16> identification;
    bit<3> flags;
    bit<13> frag_offset;
    bit<8> ttl;
    bit<8> protocol;
    bit<16> hdr_checksum;
    ip4Addr_t src_addr;
    ip4Addr_t dst_addr;
}

header tcp_t{
    bit<16> src_port;
    bit<16> dst_port;
    bit<32> seq_no;
    bit<32> ack_no;
    bit<4> data_offset;
    bit<4> res;
    bit<8> flags;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgent_ptr;
}


header udp_t{
    bit<16> src_port;
    bit<16> dst_port;
    bit<16> len;
    bit<16> checksum;
}

header rdma_base_transport_header_t {
    bit<8> opcode;
    bit<1> solicited;
    bit<1> migreq;
    bit<2> padcount;
    bit<4> version;
    bit<16> pkey;
    bit<1> fecn;
    bit<1> becn;
    bit<6> resv6;
    bit<24> dqpn;
    bit<1> ack_request;
    bit<7> resv7;
    bit<24> psn;
}

header rdma_extended_transport_t {
    bit<32> addr1;
    bit<32> addr2;
    bit<32> rkey;
    bit<32> dma_len;
}

header rdma_immdt_t {
    bit<32> immediate_data;
}

header rdma_ack_extended_transport_t {
    bit<8> syndrome;
    bit<24> msn;
}


header rdma_payload_t {
    bit<32> src_ip;
    bit<32> dst_ip;
    bit<16> src_port;
    bit<16> dst_port;
    bit<32> rtt;
    bit<32> bytes_sketch0;
    bit<32> bytes_sketch1;
    bit<32> bytes_sketch2;
    bit<32> bytes_sketch3;
    bit<32> iat;
    bit<32> pkt_type;
    bit<16> pkt_count_sketch0;
    bit<16> pkt_count_sketch1;
    bit<16> pkt_count_sketch2;
    bit<16> pkt_count_sketch3;
    bit<32> ack_no;
    bit<32> dummy;
    bit<64> timestamp;

}

header rdma_crc_t {
    bit<32> invariant_crc;
}

header mirror_bridged_metadata_t {
    pkt_type_t pkt_type;
    @flexible bit<1> do_egr_mirroring;  //  Enable egress mirroring
    @flexible MirrorId_t egr_mir_ses;   // Egress mirror session ID
    bit<8> pkt_type_tcp;
    bit<32> time;
    
    bit<32> index_sketch; // this is used as flow_id
    bit<32> reverse_index_sketch; // this is used as flow_id

    bit<8> resetting;
    
    hash_size_t index_sketch0; // this is used as flow_id
    hash_size_t index_sketch1; // this is used as flow_id
    hash_size_t index_sketch2; // this is used as flow_id
    hash_size_t index_sketch3; // this is used as flow_id
    hash_size_t reverse_index_sketch0; // this is used as flow_id
    hash_size_t reverse_index_sketch1; // this is used as flow_id
    hash_size_t reverse_index_sketch2; // this is used as flow_id
    hash_size_t reverse_index_sketch3; // this is used as flow_id

    /*
    hash_size_t index_sketch0; // this is used as flow_id
    hash_size_t index_sketch1;
    hash_size_t index_sketch2;
    hash_size_t index_sketch3;

    hash_size_t reverse_index_sketch0; // this is used as flow_id
    hash_size_t reverse_index_sketch1;
    hash_size_t reverse_index_sketch2;
    hash_size_t reverse_index_sketch3;
    */
}

header mirror_h {
   pkt_type_t pkt_type;
}

struct metadata_t {
    bit<1> do_ing_mirroring;  // Enable ingress mirroring
    bit<1> do_egr_mirroring;  // Enable egress mirroring
    MirrorId_t ing_mir_ses;   // Ingress mirror session ID
    MirrorId_t egr_mir_ses;   // Egress mirror session ID
    pkt_type_t pkt_type_mirror;

    bit<17> index_sketch; // this is used as flow_id
    hash_size_t index_sketch0; // this is used as flow_id
    hash_size_t index_sketch1;
    hash_size_t index_sketch2;
    hash_size_t index_sketch3;

     bit<17>  reverse_index_sketch; // this is used as flow_id
    hash_size_t reverse_index_sketch0; // this is used as flow_id
    hash_size_t reverse_index_sketch1;
    hash_size_t reverse_index_sketch2;
    hash_size_t reverse_index_sketch3;

    bit<17> flow_packet0;
    bit<17> flow_packet1;
    bit<17> flow_packet2;
    bit<17> flow_packet3;
    

    bit<32> bytes_sketch0;
    bit<32> bytes_sketch1;
    bit<32> bytes_sketch2;
    bit<32> bytes_sketch3;
        
    bit<16> pkts_sketch0;
    bit<16> pkts_sketch1;
    bit<16> pkts_sketch2;
    bit<16> pkts_sketch3;

    bit<32> ack_count_sketch0;
    bit<32> ack_count_sketch1;
    bit<32> ack_count_sketch2;
    bit<32> ack_count_sketch3;

    bit<1> rev_value_sketch0;
    bit<1> rev_value_sketch1;
    bit<1> rev_value_sketch2;
    bit<1> rev_value_sketch3;

    bit<32> lost;
    bit<32> iat;

    bit<32> tmp_1;
    bit<32> tmp_2;
    bit<32> tmp_3;
    bit<32> total_hdr_len_bytes; 
    bit<32> total_body_len_bytes; 
    
    bit<32> expected_ack;
    bit<32> pkt_signature;
    
    bit<16> hashed_location_1;
    bit<16> hashed_location_2;
    
    bit<32> table_1_read;
    bit<32> rtt;
    bit<32> rtt2;

    bit<32> rkey;
    bit<32> ip;
    bit<32> addr1;
    bit<32> addr2;
    bit<32> addr1_fixed;
    bit<32> addr2_fixed;
    bit<32> qpn;
    
    bit<32> sqn;
    bit<32> msn;

    bit<32> total_sent;
    bit<32> total_retr;
    bit<32> report_count;

    bit<16> offset;
    bit<16> ack_count;
    bit<1> time_period_expired;
    bit<16> rtt_exist;
    bit<32> syn_time;
    bit<32> syn_time2;
    bit<32> ack_time;
    bit<32> time;
    bit<32> total_bytes;

    bool long_flow;
    bit<32> rdma_error;
    bit<8> rdma_unknown_opcode;
    bit<16> kbytes;
    bit<32> ack_no;

    bit<16> ip_header_len;
    bit<16> tcp_header_len;
    

}

header inv_crc_h {
    bit<32>  crc;
}

header crc_values_t {
    bit<64>  lrh;
    bit<28>  classandfl;
    bit<8>   hl;
    bit<8>   resv8a;
    bit<4>   left;
}

struct headers_t {
    mirror_bridged_metadata_t           bridged_md;
    ethernet_t                          ethernet;
    ipv4_t                              ipv4;
    tcp_t                               tcp;
    udp_t                               udp;
    rdma_base_transport_header_t        rdma_bth;
    rdma_extended_transport_t           rdma_eth;
    rdma_ack_extended_transport_t       rdma_aeth;
    //rdma_immdt_t                        rdma_immdt;
    rdma_payload_t                      rdma_payload;
    inv_crc_h                           crc;
    crc_values_t                        crc_values;
}

struct paired_32bit {
    bit<32> lo;
    bit<32> hi;
}

struct sum_high_operator {\
    bit<32> old_low;\
    bit<32> high_sum;\
}\


typedef bit<8> tcp_flags_t;
const tcp_flags_t TCP_FLAGS_F = 1;
const tcp_flags_t TCP_FLAGS_S = 2;
const tcp_flags_t TCP_FLAGS_R = 4;
const tcp_flags_t TCP_FLAGS_P = 8;
const tcp_flags_t TCP_FLAGS_A = 16;
const tcp_flags_t TCP_FLAGS_U = 32; // URG flag
const tcp_flags_t TCP_FLAGS_E = 64; // ECE flag
const tcp_flags_t TCP_FLAGS_C = 128; // CWR flag

#define PKT_TYPE_SYN 0
#define PKT_TYPE_SEQ 1
#define PKT_TYPE_ACK 2
#define PKT_TYPE_SYNACK 3
#define PKT_TYPE_FIN_RST 4

struct flow_digest_t {
	hash_size_t flow_id;
    ip4Addr_t src_ip;
	ip4Addr_t dst_ip;
	bit<16> src_port;
	bit<16> dst_port;
	bit<8> protocol;
} 

struct flow_location_digest_t {
    bit<17> index_sketch;
    bit<17> reverse_index_sketch;
	hash_size_t index0;
    hash_size_t index1;
	hash_size_t index2;
	hash_size_t index3;
}  


struct bytes_digest_t {
    bit<32> bytes_sketch0;
    bit<32> bytes_sketch1;
    bit<32> bytes_sketch2;
    bit<32> bytes_sketch3;
}  

struct ack_count_digest_t {
    bit<32> ack_count_sketch0;
    bit<32> ack_count_sketch1;
    bit<32> ack_count_sketch2;
    bit<32> ack_count_sketch3;
}  



struct flow_metric_update_digest_t {
    hash_size_t flow_id;
    ip4Addr_t src_ip;
	ip4Addr_t dst_ip;
	bit<16> src_port;
	bit<16> dst_port;
	bit<8> protocol;
    bit<32> rtt;
}

struct rdma_reset_addr_t {
    bit<16> reset;
}

/********  G L O B A L   E G R E S S   M E T A D A T A  *********/

struct my_egress_headers_t {
}
struct my_egress_metadata_t {
}
