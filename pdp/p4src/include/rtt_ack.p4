#include "./registers.p4"

control RTTAck(inout headers_t hdr, inout metadata_t meta, in egress_intrinsic_metadata_from_parser_t eg_prsr_md) {
    Hash<bit<32>>(HashAlgorithm_t.CRC32) crc32_1;
    Hash<bit<32>>(HashAlgorithm_t.CRC32) crc32_2;
    Hash<bit<16>>(HashAlgorithm_t.CRC16) crc16_1;
    Hash<bit<16>>(HashAlgorithm_t.CRC16) crc16_2;

    
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
    action get_pkt_signature_ACK(){
        meta.pkt_signature=crc32_2.get({
            hdr.ipv4.dst_addr,hdr.ipv4.src_addr, 
            hdr.tcp.dst_port,hdr.tcp.src_port, 
            hdr.tcp.ack_no
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

    apply {
        get_pkt_signature_ACK();
        get_location_ACK();
        exec_table_1_tryRead();
        if(meta.table_1_read != 0){
            //meta.rtt 
            hdr.rdma_payload.rtt = eg_prsr_md.global_tstamp[31:0]-meta.table_1_read; 
            //meta.rtt_exist = 1 ;
        }
    }

}