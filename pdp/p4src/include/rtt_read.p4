#include "./registers.p4"

control RTTRead(inout headers_t hdr, inout metadata_t meta) {    
    RegisterAction<bit<32>, bit<16>, bit<32>>(rtts) read_rtt = {
        void apply(inout bit<32> register_data, out bit<32> result) {
            result = register_data;
        }
    };

    apply {
        meta.rtt = read_rtt.execute(meta.reverse_index_sketch0);
    }
}