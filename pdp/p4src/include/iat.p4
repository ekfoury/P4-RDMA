control IAT(inout headers_t hdr, inout metadata_t meta, in egress_intrinsic_metadata_from_parser_t eg_prsr_md) {

    Register<bit<32>, bit<17>>(131072) iat_last_timestamp;
    Register<bit<32>, bit<17>>(131072) iat;

    RegisterAction<bit<32>, bit<17>, bit<32>>(iat) update_iat = {
        void apply(inout bit<32> register_data, out bit<32> result) { 
            register_data = meta.iat;
        } 
    }; 

    RegisterAction<bit<32>, bit<17>, bit<32>>(iat) get_iat_ack = {
        void apply(inout bit<32> register_data, out bit<32> result) { 
            result = register_data;
        } 
    }; 


    RegisterAction<bit<32>, bit<17>, bit<32>>(iat_last_timestamp) get_iat = {
        void apply(inout bit<32> register_data, out bit<32> result) { 
            bit<32> time = eg_prsr_md.global_tstamp[31:0];
            if(register_data != 0) {
                result = time - register_data;
                register_data = time;
            } else {
                register_data = time;
            }
        } 
    }; 

    apply {
        if(hdr.bridged_md.pkt_type_tcp != PKT_TYPE_ACK) { 
            meta.iat = get_iat.execute(meta.index_sketch);
            update_iat.execute(meta.index_sketch);
        } else {
            hdr.rdma_payload.iat = get_iat_ack.execute(meta.reverse_index_sketch);
        }
    }
}