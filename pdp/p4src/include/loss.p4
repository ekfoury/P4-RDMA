
control Loss(inout headers_t hdr, inout metadata_t meta) {
    Register<bit<32>, bit<17>>(131072) seq_reg;

    
    RegisterAction<bit<32>, bit<17>, bit<32>>(seq_reg) update_seq_reg = {
        void apply(inout bit<32> register_data, out bit<32> result) {
            register_data = hdr.tcp.seq_no;
        }
    };
     RegisterAction<bit<32>, bit<17>, bit<32>>(seq_reg) get_seq_reg = {
        void apply(inout bit<32> register_data, out bit<32> result) {
            result = register_data;
        }
    };
    
    apply {
        if(hdr.bridged_md.pkt_type_tcp==PKT_TYPE_ACK) {
            hdr.rdma_payload.dummy = get_seq_reg.execute(meta.reverse_index_sketch);
        } else if(hdr.bridged_md.pkt_type_tcp==PKT_TYPE_SEQ){
            update_seq_reg.execute(meta.index_sketch);
        }
    }

}




