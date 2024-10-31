typedef bit<16> packet_size_value_t;
typedef bit<17> index_t;


control PacketSize(inout headers_t hdr, inout metadata_t meta) {
    Register<bit<16>, bit<16>>(1024) packet_size;

    Lpf<packet_size_value_t, index_t>(size=32768) lpf_1;
    Lpf<packet_size_value_t, index_t>(size=32768) lpf_2;

    packet_size_value_t lpf_input;
    packet_size_value_t lpf_output_1;
    packet_size_value_t lpf_output_2;

    RegisterAction<bit<16>, bit<16>, bit<16>>(packet_size) update_packet_size = {
        void apply(inout bit<16> register_data, out bit<16> result) {
                register_data = hdr.ipv4.total_len;
        }
    };
     RegisterAction<bit<16>, bit<16>, bit<16>>(packet_size) get_packet_size = {
        void apply(inout bit<16> register_data, out bit<16> result) {
            result = register_data;
        }
    };
    


    
    apply {
        if (hdr.bridged_md.pkt_type_tcp != PKT_TYPE_ACK)  {
            lpf_input = (packet_size_value_t)hdr.ipv4.total_len;
            lpf_output_1 = lpf_1.execute(lpf_input, 0);

            //update_packet_size.execute(meta.index_sketch);
        } else {
            //lpf_input = (packet_size_value_t)hdr.ipv4.total_len;
            //lpf_output_1 = lpf_1.execute(lpf_input, 0);
           
            //hdr.rdma_payload.ack_count_sketch2 = (bit<32>)lpf_output_1; //  (bit<32>)get_packet_size.execute(meta.reverse_index_sketch);
        }
        
    }
}