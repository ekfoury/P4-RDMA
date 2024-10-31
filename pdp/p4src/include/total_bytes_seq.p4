#include "./registers.p4"

control TotalBytesSeq(inout headers_t hdr, inout metadata_t meta) {

    RegisterAction<bit<32>, bit<17>, bit<32>>(total_bytes) update_total_bytes = {
        void apply(inout bit<32> register_data) {
            register_data = register_data + (bit<32>)hdr.ipv4.total_len;
        }
    };
    RegisterAction<bit<32>, bit<17>, bit<32>>(total_bytes) get_total_bytes = {
        void apply(inout bit<32> register_data, out bit<32> result) {
            result = register_data;
        }
    };

    apply {
        if(SEQ_PACKET){
        update_total_bytes.execute(meta.index_sketch);
        } else {
        meta.total_bytes = get_total_bytes.execute(meta.reverse_index_sketch);

        }
    }
}