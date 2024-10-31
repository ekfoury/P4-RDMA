#include "./registers.p4"

control PacketLossSeq(inout headers_t hdr, inout metadata_t meta, inout ingress_intrinsic_metadata_for_deparser_t  ig_intr_dprsr_md) {
    #define ACK_SKETCH(num) Register<bit<SKETCH_CELL_BIT_WIDTH>, bit<17>>(131072) ack_sketch##num; 
    
    #define HASH_FLOW_PACKET(num) Hash<bit<17>>(HashAlgorithm_t.CUSTOM, crc17d_##num) hash_flow_packet##num; \
    action apply_hash_flow_packet_##num() { \
        meta.flow_packet##num = hash_flow_packet##num.get({ \
            hdr.ipv4.dst_addr, \
            hdr.ipv4.src_addr, \
            hdr.ipv4.protocol, \
            hdr.tcp.dst_port, \
            hdr.tcp.src_port,\
            hdr.tcp.ack_no \
        }); \
    }

    #define ACK_SKETCH_REGISTER_UPDATE_ACTION(num) RegisterAction<bit<SKETCH_CELL_BIT_WIDTH>, bit<17>, bit<SKETCH_CELL_BIT_WIDTH>>(ack_sketch##num) update_ack_sketch##num = {\
        void apply(inout bit<SKETCH_CELL_BIT_WIDTH> register_data, out bit<SKETCH_CELL_BIT_WIDTH> result) { \
            register_data = register_data + 1; \
            result = register_data; \
        } \
    }; \
    action exec_update_ack_sketch_##num() { \
        meta.ack_count_sketch##num = update_ack_sketch##num.execute(meta.flow_packet##num); \
    }



    HASH_FLOW_PACKET(0) 
    HASH_FLOW_PACKET(1) 
    HASH_FLOW_PACKET(2) 
    HASH_FLOW_PACKET(3) 

    ACK_SKETCH(0)
    ACK_SKETCH(1)
    ACK_SKETCH(2)
    ACK_SKETCH(3)

    ACK_SKETCH_REGISTER_UPDATE_ACTION(0)
    ACK_SKETCH_REGISTER_UPDATE_ACTION(1)
    ACK_SKETCH_REGISTER_UPDATE_ACTION(2)
    ACK_SKETCH_REGISTER_UPDATE_ACTION(3)
    
    apply {
        if(hdr.bridged_md.pkt_type_tcp == PKT_TYPE_ACK) {
            apply_hash_flow_packet_0();
            apply_hash_flow_packet_1();
            apply_hash_flow_packet_2();
            apply_hash_flow_packet_3();
            
            exec_update_ack_sketch_0();
            exec_update_ack_sketch_1();
            exec_update_ack_sketch_2();
            exec_update_ack_sketch_3();
            
            //ig_intr_dprsr_md.digest_type = 1;
        }     
    }
}