
#include "./registers.p4"

control TotalBytes(inout headers_t hdr, inout metadata_t meta,inout ingress_intrinsic_metadata_for_deparser_t  ig_intr_dprsr_md) {
    #define SKETCH_REGISTER_UPDATE_ACTION(num) RegisterAction<bit<SKETCH_CELL_BIT_WIDTH>, bit<16>, bit<SKETCH_CELL_BIT_WIDTH>>(sketch##num) update_sketch##num = {\
        void apply(inout bit<SKETCH_CELL_BIT_WIDTH> register_data, out bit<SKETCH_CELL_BIT_WIDTH> result) { \
            register_data = register_data + (bit<32>)(meta.kbytes); \
        } \
    }; \
    action exec_update_sketch##num() { \
        update_sketch##num.execute(meta.index_sketch##num); \
    }

    #define SKETCH_REGISTER_READ_ACTION(num) RegisterAction<bit<SKETCH_CELL_BIT_WIDTH>, bit<16>, bit<SKETCH_CELL_BIT_WIDTH>>(sketch##num) read_sketch_reverse##num = {\
        void apply(inout bit<SKETCH_CELL_BIT_WIDTH> register_data, out bit<SKETCH_CELL_BIT_WIDTH> result) { \
            result = register_data;\
        } \
    }; 

    #define READ_LOGIC(num) meta.bytes_sketch##num = read_sketch_reverse##num.execute(meta.reverse_index_sketch##num); 



    SKETCH_REGISTER_UPDATE_ACTION(0)
    SKETCH_REGISTER_UPDATE_ACTION(1)
    SKETCH_REGISTER_UPDATE_ACTION(2)
    SKETCH_REGISTER_UPDATE_ACTION(3)

    SKETCH_REGISTER_READ_ACTION(0)
    SKETCH_REGISTER_READ_ACTION(1)
    SKETCH_REGISTER_READ_ACTION(2)
    SKETCH_REGISTER_READ_ACTION(3)

    bit<16> tmp; 
    action set_bytes(bit<16> headers_bytes) {
        tmp =  headers_bytes;
    }

    table tb_calculate_bytes {
        key = {
            hdr.ipv4.ihl: exact;
            hdr.tcp.data_offset: exact;
        }
        actions = {
            set_bytes();
        }
        default_action = set_bytes(40);
        size = 128;
    }

    
    apply {
        if(hdr.bridged_md.pkt_type_tcp==PKT_TYPE_ACK) {
            READ_LOGIC(0)
            READ_LOGIC(1)
            READ_LOGIC(2)
            READ_LOGIC(3)
        } else {
            //tb_calculate_bytes.apply();
            meta.kbytes = hdr.ipv4.total_len; // - tmp;
            exec_update_sketch0();
            exec_update_sketch1();
            exec_update_sketch2();
            exec_update_sketch3();
        }
    }

}




