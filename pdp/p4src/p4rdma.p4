/* -*- P4_16 -*- */

#include <core.p4>
#include <tna.p4>
#include "include/headers.p4"
#include "include/ingress_parser.p4"
#include "include/ingress_deparser.p4"
#include "include/egress_parser.p4"
#include "include/egress_deparser.p4"
#include "include/total_bytes.p4"
#include "include/long_flow_reverse.p4"
#include "include/packet_type.p4"
#include "include/packet_loss_seq.p4"
#include "include/packet_loss_ack.p4"
#include "include/rdma.p4"
#include "include/rdma_msn_update.p4"
#include "include/rdma_msn_read.p4"
#include "include/mirror_ingress.p4"
#include "include/mirror_egress.p4"
#include "include/icrc_calculator.p4"
#include "include/rtt_handshake_syn.p4"
#include "include/total_packets.p4"
#include "include/flow_hashing.p4"
#include "include/iat.p4"
#include "include/packet_size.p4"
#include "include/loss.p4"


/*************************************************************************
 **************  I N G R E S S   P R O C E S S I N G   *******************
 *************************************************************************/

control Ingress(
    /* User */
    inout headers_t                       hdr,
    inout metadata_t                      meta,
    /* Intrinsic */
    in    ingress_intrinsic_metadata_t               ig_intr_md,
    in    ingress_intrinsic_metadata_from_parser_t   ig_prsr_md,
    inout ingress_intrinsic_metadata_for_deparser_t  ig_intr_dprsr_md,
    inout ingress_intrinsic_metadata_for_tm_t        ig_tm_md)
{
    TotalBytes() total_bytes_logic;
    TotalPackets() total_packets_logic;
    PacketType() determine_tcp_packet_type;
    PacketLossSeq() packet_loss;
    FlowHashing() flow_hashing;
    RDMA()      rdma;
    RDMAMsnUpdate()   rdma_msn_update;
    MirrorIngress()      mirror_ingress;
   RTTHandshakeSyn() rtt_handshake;
    PacketSize() packet_size;
    Loss() loss;
    
    action set_normal_pkt() {
        hdr.bridged_md.setValid();
        hdr.bridged_md.pkt_type = PKT_TYPE_NORMAL;
    }


    RegisterAction<bit<32>, bit<1>, bit<32>>(rdma_psn) read_psn = {
        void apply(inout bit<32> register_data, out bit<32> result) { 
            result = register_data;  
            register_data = register_data + 1;   
        } 
    }; 
    action exec_read_psn() { 
        hdr.rdma_bth.psn = read_psn.execute(0)[23:0]; // + 1; 
    }
    
    
    RegisterAction<bit<32>, bit<1>, bit<32>>(rdma_psn) adjust_psn = {
        void apply(inout bit<32> register_data, out bit<32> result) { 
            register_data = (bit<32>)meta.rdma_error + 1; 
        } 
    }; 
    action exec_adjust_psn() { 
        adjust_psn.execute(0); // + 1; 
    }


    RegisterAction<bit<32>, bit<1>, bit<32>>(rdma_msn) update_msn = {
        void apply(inout bit<32> register_data, out bit<32> result) { 
                register_data = (bit<32>)hdr.rdma_aeth.msn;  
                result = 1;
        } 
    }; 

    RegisterAction<bit<32>, bit<1>, bit<32>>(retransmitted_rdma) update_retransmitted = {
        void apply(inout bit<32> register_data, out bit<32> result) { 
            if(meta.rdma_error == 1) {
                register_data = (bit<32>)hdr.rdma_bth.psn;
            } else {
                register_data = 0;
            }
        } 
    }; 

    RegisterAction<bit<32>, bit<1>, bit<32>>(retransmitted_rdma) read_retransmitted = {
        void apply(inout bit<32> register_data, out bit<32> result) { 
            result = register_data;
        } 
    }; 



    apply {

        
        if(FROM_CPU) {
            SEND_TO(COLLECTOR_PORT);
        } else if(FROM_COLLECTOR) {  
            SEND_TO(CPU_PORT)   
        } else {
            set_normal_pkt();
            meta.time = ig_intr_md.ingress_mac_tstamp[41:10];
            if(TCP_PACKET) {
                determine_tcp_packet_type.apply(hdr, meta);
                meta.ack_no = hdr.tcp.ack_no;

                if(hdr.bridged_md.pkt_type_tcp == PKT_TYPE_ACK ) {
                    exec_read_psn();
                }
                
                flow_hashing.apply(hdr, meta);
                rtt_handshake.apply(hdr, meta);

                total_bytes_logic.apply(hdr, meta, ig_intr_dprsr_md);  
                total_packets_logic.apply(hdr, meta, ig_intr_dprsr_md); 
                loss.apply(hdr, meta);
                rdma.apply(hdr, meta, ig_intr_md, ig_tm_md, ig_intr_dprsr_md); 
                
                hdr.rdma_payload.ack_no = hdr.tcp.ack_no;
            }

        }
    }
}

/*************************************************************************
 ***************   E G R E S S   P R O C E S S I N G   *******************
 *************************************************************************/

control Egress(
    /* User */
    inout headers_t                          hdr,
    inout metadata_t                         meta,
    /* Intrinsic */
    in    egress_intrinsic_metadata_t                  eg_intr_md,
    in    egress_intrinsic_metadata_from_parser_t      eg_prsr_md,
    inout egress_intrinsic_metadata_for_deparser_t     eg_dprsr_md,
    inout egress_intrinsic_metadata_for_output_port_t  eg_oport_md)
{
    ICRCCalculator() icrc_calculator;
    MirrorEgress() mirror_egress;
    PacketType() determine_tcp_packet_type;
    FlowHashing() flow_hashing;
    IAT() iat;

    #define SKETCH_REGISTER_PACKET_COUNT(num) Register<bit<SKETCH_CELL_BIT_WIDTH>, bit<16>>(SKETCH_BUCKET_LENGTH) sketch_packet_count##num; 
    SKETCH_REGISTER_PACKET_COUNT(0)
    SKETCH_REGISTER_PACKET_COUNT(1)
    SKETCH_REGISTER_PACKET_COUNT(2)
    SKETCH_REGISTER_PACKET_COUNT(3)

    #define SKETCH_REGISTER_UPDATE_ACTION_COUNT(num) RegisterAction<bit<SKETCH_CELL_BIT_WIDTH>, bit<16>, bit<SKETCH_CELL_BIT_WIDTH>>(sketch_packet_count##num) update_sketch_packet_count##num = {\
        void apply(inout bit<SKETCH_CELL_BIT_WIDTH> register_data, out bit<SKETCH_CELL_BIT_WIDTH> result) { \
            register_data = register_data + 1; \
        } \
    }; \
    action exec_update_sketch_packet_count##num() { \
        update_sketch_packet_count##num.execute(meta.index_sketch##num); \
    }

    #define SKETCH_REGISTER_READ_ACTION_COUNT(num) RegisterAction<bit<SKETCH_CELL_BIT_WIDTH>, bit<16>, bit<SKETCH_CELL_BIT_WIDTH>>(sketch_packet_count##num) read_sketch_reverse_packet_count##num = {\
        void apply(inout bit<SKETCH_CELL_BIT_WIDTH> register_data, out bit<SKETCH_CELL_BIT_WIDTH> result) { \
            result = register_data;\
        } \
    }; 

    #define READ_LOGIC_COUNT_PACKET_COUNT(num) hdr.rdma_payload.packet_count##num = read_sketch_reverse_packet_count##num.execute(meta.reverse_index_sketch##num); 


    SKETCH_REGISTER_UPDATE_ACTION_COUNT(0)
    SKETCH_REGISTER_UPDATE_ACTION_COUNT(1)
    SKETCH_REGISTER_UPDATE_ACTION_COUNT(2)
    SKETCH_REGISTER_UPDATE_ACTION_COUNT(3)

    SKETCH_REGISTER_READ_ACTION_COUNT(0)
    SKETCH_REGISTER_READ_ACTION_COUNT(1)
    SKETCH_REGISTER_READ_ACTION_COUNT(2)
    SKETCH_REGISTER_READ_ACTION_COUNT(3)

    
    apply {

        iat.apply(hdr, meta, eg_prsr_md);

        if (hdr.bridged_md.do_egr_mirroring == 1 && hdr.bridged_md.resetting == 0) { // && (hdr.rdma_payload.rtt[15:0] != 0 || hdr.rdma_payload.rtt2[15:0] != 0)) {
            icrc_calculator.apply(hdr, meta);
            mirror_egress.apply(hdr, meta, eg_dprsr_md);
        }
    }
}


/************ F I N A L   P A C K A G E ******************************/
Pipeline(
    IngressParser(),
    Ingress(),
    IngressDeparser(),
    EgressParser(),
    Egress(),
    EgressDeparser()
) pipe;

Switch(pipe) main;
