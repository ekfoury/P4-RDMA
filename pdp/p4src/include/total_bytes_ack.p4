#include "./registers.p4"

control TotalBytesAck(inout headers_t hdr, inout metadata_t meta) {

    RegisterAction<bit<32>, bit<16>, bit<32>>(total_bytes) get_total_bytes = {
        void apply(inout bit<32> register_data, out bit<32> result) {
            result = register_data;
        }
    };

    apply {
        meta.total_bytes = get_total_bytes.execute(meta.reverse_index_sketch0);
    }
}