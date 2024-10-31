
#include "./registers.p4"

control TotalPackets(inout headers_t hdr, inout metadata_t meta,inout ingress_intrinsic_metadata_for_deparser_t  ig_intr_dprsr_md) {
    #define SKETCH_REGISTER_UPDATE_ACTION_PACKETS(num) RegisterAction<bit<SKETCH_CELL_BIT_WIDTH_PACKETS>, bit<16>, bit<SKETCH_CELL_BIT_WIDTH_PACKETS>>(sketch_packets##num) update_sketch_packets##num = {\
        void apply(inout bit<SKETCH_CELL_BIT_WIDTH_PACKETS> register_data, out bit<SKETCH_CELL_BIT_WIDTH_PACKETS> result) { \
            register_data = register_data + 1; \
        } \
    }; \
    action exec_update_sketch_packets##num() { \
        update_sketch_packets##num.execute(meta.index_sketch##num); \
    }

    #define SKETCH_REGISTER_READ_ACTION_PACKETS(num) RegisterAction<bit<SKETCH_CELL_BIT_WIDTH_PACKETS>, bit<16>, bit<SKETCH_CELL_BIT_WIDTH_PACKETS>>(sketch_packets##num) read_sketch_reverse_packets##num = {\
        void apply(inout bit<SKETCH_CELL_BIT_WIDTH_PACKETS> register_data, out bit<SKETCH_CELL_BIT_WIDTH_PACKETS> result) { \
            result = register_data;\
        } \
    }; 

    #define READ_LOGIC_PACKETS(num) meta.pkts_sketch##num = read_sketch_reverse_packets##num.execute(meta.reverse_index_sketch##num); 



    SKETCH_REGISTER_UPDATE_ACTION_PACKETS(0)
    SKETCH_REGISTER_UPDATE_ACTION_PACKETS(1)
    SKETCH_REGISTER_UPDATE_ACTION_PACKETS(2)
    SKETCH_REGISTER_UPDATE_ACTION_PACKETS(3)

    SKETCH_REGISTER_READ_ACTION_PACKETS(0)
    SKETCH_REGISTER_READ_ACTION_PACKETS(1)
    SKETCH_REGISTER_READ_ACTION_PACKETS(2)
    SKETCH_REGISTER_READ_ACTION_PACKETS(3)


    
    apply {
        if(hdr.bridged_md.pkt_type_tcp==PKT_TYPE_ACK) {
            READ_LOGIC_PACKETS(0)
            READ_LOGIC_PACKETS(1)
            READ_LOGIC_PACKETS(2)
            READ_LOGIC_PACKETS(3)
        } else {
            exec_update_sketch_packets0();
            exec_update_sketch_packets1();
            exec_update_sketch_packets2();
            exec_update_sketch_packets3();
        }
    }

}




