
control ICRCCalculator(inout headers_t hdr, inout metadata_t meta) {
    bit<32> tmp1 = 0; 
    bit<32> tmp2 = 0; 
    bit<32> tmp3 = 0;  
    bit<32> tmp4 = 0;  
    bit<64> tmp5 = 0; 
    CRCPolynomial<bit<32>>(    
        coeff = 0x04C11DB7,    
        reversed = true,       
        msb = false,           
        extended = false,      
        init = 0xFFFFFFFF,     
        xor = 0xFFFFFFFF) poly;
    Hash<bit<32>>(HashAlgorithm_t.CUSTOM, poly) crc_hash;
    action calculate_crc() {                
        hdr.crc.setValid();                 
        hdr.crc.crc = crc_hash.get({                
            (bit<64>)0xFFFFFFFFFFFFFFFF,                
            hdr.ipv4.version,                
            hdr.ipv4.ihl,                
            (bit<8>)0xFF,                 
            hdr.ipv4.total_len,                
            hdr.ipv4.identification,                
            hdr.ipv4.flags,                
            hdr.ipv4.frag_offset,                
            (bit<8>)0xFF,                 
            hdr.ipv4.protocol,                
            (bit<16>)0xFFFF,                 
            hdr.ipv4.src_addr,                
            hdr.ipv4.dst_addr,                
            hdr.udp.src_port,                
            hdr.udp.dst_port,                
            hdr.udp.len,                
            (bit<16>)0xFFFF,                 
            hdr.rdma_bth.opcode,                
            hdr.rdma_bth.solicited,                
            hdr.rdma_bth.migreq,                
            hdr.rdma_bth.padcount,                
            hdr.rdma_bth.version,                
            hdr.rdma_bth.pkey,                
            (bit<1>)1,                
            (bit<1>)1,                
            (bit<6>)0x3F,                    
            hdr.rdma_bth.dqpn,                
            hdr.rdma_bth.ack_request,                
            hdr.rdma_bth.resv7,                
            hdr.rdma_bth.psn,                
            hdr.rdma_eth.addr1,                
            hdr.rdma_eth.addr2,                
            hdr.rdma_eth.rkey,                
            hdr.rdma_eth.dma_len,                
            hdr.rdma_payload.src_ip,                
            hdr.rdma_payload.dst_ip,                
            hdr.rdma_payload.src_port,                
            hdr.rdma_payload.dst_port,                
            hdr.rdma_payload.rtt,                
            hdr.rdma_payload.bytes_sketch0,
            hdr.rdma_payload.bytes_sketch1, 
            hdr.rdma_payload.bytes_sketch2, 
            hdr.rdma_payload.bytes_sketch3, 
            hdr.rdma_payload.iat, 
            hdr.rdma_payload.pkt_type, 
            hdr.rdma_payload.pkt_count_sketch0, 
            hdr.rdma_payload.pkt_count_sketch1,
            hdr.rdma_payload.pkt_count_sketch2, 
            hdr.rdma_payload.pkt_count_sketch3,
            hdr.rdma_payload.ack_no,
            hdr.rdma_payload.dummy,
            hdr.rdma_payload.timestamp
        });                
    }	                
    action swap_crc() {                
        tmp1 = hdr.crc.crc & 0x000000FF;                
        tmp2 = hdr.crc.crc & 0x0000FF00;                
        tmp3 = hdr.crc.crc & 0x00FF0000;                
        tmp4 = hdr.crc.crc & 0xFF000000;                
    }                
    action swap2_crc() {                
        tmp1 = tmp1 << 24;                
        tmp2 = tmp2 << 8;                
        tmp3 = tmp3 >> 8;                
        tmp4 = tmp4 >> 24;                
    }                
    action swap3_crc() {                
        tmp1 = tmp1 | tmp2;                
        tmp3 = tmp3 | tmp4;                
    }                                      
    action swap4_crc() {                
        hdr.crc.crc = tmp1 | tmp3;       
    }  

    apply {
        calculate_crc(); 
        hdr.crc.crc = hdr.crc.crc & 0xffffffff;   
        swap_crc();                               
        swap2_crc();                              
        swap3_crc();                              
        swap4_crc();
    }                                 
}