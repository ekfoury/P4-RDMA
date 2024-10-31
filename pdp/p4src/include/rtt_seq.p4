#include "./registers.p4"

control RTTSeq(inout headers_t hdr, inout metadata_t meta, in egress_intrinsic_metadata_from_parser_t eg_prsr_md) {

    Hash<bit<32>>(HashAlgorithm_t.IDENTITY) copy32_1;
    Hash<bit<32>>(HashAlgorithm_t.IDENTITY) copy32_2;
    action compute_eack_1_(){
        meta.tmp_1=copy32_1.get({26w0 ++ hdr.ipv4.ihl ++ 2w0});
    }
    action compute_eack_2_(){
        meta.tmp_2=copy32_2.get({26w0 ++ hdr.tcp.data_offset ++ 2w0});
    }
    action compute_eack_3_(){
        meta.tmp_3=16w0 ++ hdr.ipv4.total_len;
    }
    action compute_eack_4_(){
        meta.total_hdr_len_bytes=(meta.tmp_1+meta.tmp_2);
    }
    action compute_eack_5_(){
        meta.total_body_len_bytes=meta.tmp_3 - meta.total_hdr_len_bytes;
    }
    action compute_eack_6_(){
        meta.expected_ack=hdr.tcp.seq_no + meta.total_body_len_bytes;
    }
    action compute_eack_last_if_syn(){
        meta.expected_ack=meta.expected_ack + 1;
    }
    Hash<bit<32>>(HashAlgorithm_t.CRC32) crc32_1;
    Hash<bit<32>>(HashAlgorithm_t.CRC32) crc32_2;
    action get_pkt_signature_SEQ(){
        meta.pkt_signature=crc32_1.get({
            hdr.ipv4.src_addr, hdr.ipv4.dst_addr,
            hdr.tcp.src_port, hdr.tcp.dst_port,
            meta.expected_ack
        });
    }
    action get_pkt_signature_ACK(){
        meta.pkt_signature=crc32_2.get({
            hdr.ipv4.dst_addr,hdr.ipv4.src_addr, 
            hdr.tcp.dst_port,hdr.tcp.src_port, 
            hdr.tcp.ack_no
        });
    }
    Hash<bit<16>>(HashAlgorithm_t.CRC16) crc16_1;
    Hash<bit<16>>(HashAlgorithm_t.CRC16) crc16_2;
    action get_location_SEQ(){
        meta.hashed_location_1=crc16_1.get({        
            4w0,                                    
            hdr.ipv4.src_addr, hdr.ipv4.dst_addr,   
            hdr.tcp.src_port, hdr.tcp.dst_port,     
            meta.expected_ack,                     
            4w0                                     
        });
    }
    action get_location_ACK(){
        meta.hashed_location_1=crc16_2.get({
            4w0,                                
            hdr.ipv4.dst_addr,hdr.ipv4.src_addr, 
            hdr.tcp.dst_port,hdr.tcp.src_port, 
            hdr.tcp.ack_no,                      
            4w0                                  
        });
    }
    RegisterAction<paired_32bit, bit<16>, bit<32>>(reg_table_1) table_1_insert= {  
        void apply(inout paired_32bit value, out bit<32> rv) {               
            rv = 0;                                                          
            paired_32bit in_value;                                           
            in_value = value;                                                   
            bool existing_timestamp_is_old = (eg_prsr_md.global_tstamp[31:0]-in_value.hi)>(50000000);
            bool current_entry_empty = in_value.lo==0;                          
            if(existing_timestamp_is_old || current_entry_empty)                
            {                                                                   
                value.lo=meta.pkt_signature;                                   
                value.hi=eg_prsr_md.global_tstamp[31:0];                   
                rv=1;
            }
        }                                                              
    };
    action exec_table_1_insert(){
        meta.table_1_read=table_1_insert.execute(meta.hashed_location_1);
    }
    RegisterAction<paired_32bit, bit<16>, bit<32>>(reg_table_1) table_1_tryRead= {  
        void apply(inout paired_32bit value, out bit<32> rv) {    
            rv=0;
            paired_32bit in_value;                                          
            in_value = value;     
            if((in_value.lo==meta.pkt_signature) && ((eg_prsr_md.global_tstamp[31:0]-in_value.hi)< 2000000000 ))
            {
                value.lo=0;
                value.hi=0;
                rv=in_value.hi;
            }
        }                                                    
    };
    action exec_table_1_tryRead(){
        meta.table_1_read=table_1_tryRead.execute(meta.hashed_location_1);
    }

    bit<1> time_period_expired;
    Register<bit<16>, bit<16>>(65535) last_period_timestamp; 
        RegisterAction<bit<16>, bit<16>, bit<1>>(last_period_timestamp) update_last_report_timestamp = {
        void apply(inout bit<16> register_data, out bit<1> result) {
                if (register_data == 0) {
                    register_data = eg_prsr_md.global_tstamp[42:27];
                } else {
                bit<16> tmp;
                tmp = eg_prsr_md.global_tstamp[42:27] - register_data;
                if(tmp > 3) { 
                    register_data = eg_prsr_md.global_tstamp[42:27];
                    result = 1;
                } else {
                    result = 0;
                }
            }
        }
    };
    action apply_update_last_report_timestamp() {
        meta.time_period_expired = update_last_report_timestamp.execute(meta.reverse_index_sketch0);
    }
    Register<bit<32>, bit<16>>(65536) flow_last_rtt; 
    RegisterAction<bit<32>, bit<16>, bit<32>>(flow_last_rtt) update_flow_last_rtt = {
        void apply(inout bit<32> register_data, out bit<32> result) { 
            register_data = meta.rtt;                      
        } 
    }; 
    action exec_update_flow_last_rtt() { 
        update_flow_last_rtt.execute(meta.reverse_index_sketch0); 
    }
    RegisterAction<bit<32>, bit<16>, bit<32>>(flow_last_rtt) get_flow_last_rtt = {
        void apply(inout bit<32> register_data, out bit<32> result) { 
            result = register_data;                      
        } 
    }; 
    action exec_get_flow_last_rtt() { 
        meta.rtt = get_flow_last_rtt.execute(meta.hashed_location_1); 
    }

    RegisterAction<bit<16>, bit<16>, bit<16>>(count_ack) update_count_ack = {
        void apply(inout bit<16> register_data, out bit<16> result) { 
            bit<16> tmp = register_data;
            if (tmp < 20){
                register_data = register_data+ 1; 
            } else if (meta.rtt_exist != 0) { 
                result = 20;
                register_data = 0;
            }
        } 
    }; 
    action exec_update_count_ack() { 
        meta.ack_count = update_count_ack.execute(meta.reverse_index_sketch0); 
    }

    apply {
        compute_eack_1_();
        compute_eack_2_();
        compute_eack_3_();
        compute_eack_4_();
        compute_eack_5_();
        compute_eack_6_();
        if(hdr.tcp.flags==TCP_FLAGS_S){
            compute_eack_last_if_syn();
        }
        get_pkt_signature_SEQ();
        get_location_SEQ();
        exec_table_1_insert();
    }
}

