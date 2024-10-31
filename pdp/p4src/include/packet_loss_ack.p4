#include "./registers.p4"

control PacketLossAck(inout headers_t hdr, inout metadata_t meta) {
    
    RegisterAction<bit<32>, bit<16>, bit<32>>(total_sent) get_total_sent = {
        void apply(inout bit<32> register_data, out bit<32> result) {
            result = register_data;
        }
    };
    action exec_get_total_sent(){
        meta.total_sent = get_total_sent.execute(meta.reverse_index_sketch0);
	}
    RegisterAction<bit<32>, bit<16>, bit<32>>(total_retr) get_total_retr = {
        void apply(inout bit<32> register_data, out bit<32> result) {
            result = register_data;
        }
    };
    action exec_get_total_retr(){
        meta.total_retr = get_total_retr.execute(meta.reverse_index_sketch0);
	}
    
    apply {
        exec_get_total_sent();
        exec_get_total_retr();
    }
}
