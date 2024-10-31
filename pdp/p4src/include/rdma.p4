#include "./registers.p4"

control RDMA (inout headers_t hdr, inout metadata_t meta, in ingress_intrinsic_metadata_t ig_intr_md,inout ingress_intrinsic_metadata_for_tm_t ig_tm_md, inout ingress_intrinsic_metadata_for_deparser_t  ig_intr_dprsr_md) {
    Register<bit<32>, bit<1>>(1) rdma_addr_2_fixed; 
    RegisterAction<bit<32>, bit<1>, bit<32>>(rdma_addr_2_fixed) read_rdma_addr_2_fixed = {
        void apply(inout bit<32> register_data, out bit<32> result) { 
            result = register_data;            
        } 
    }; 
    action exec_read_rdma_addr_2_fixed() { 
        meta.addr2_fixed = read_rdma_addr_2_fixed.execute(0); 
    }

    Register<bit<32>, bit<1>>(1) rdma_addr_1_fixed; 
    RegisterAction<bit<32>, bit<1>, bit<32>>(rdma_addr_1_fixed) read_rdma_addr_1_fixed = {
        void apply(inout bit<32> register_data, out bit<32> result) { 
            result = register_data;            
        } 
    }; 
    action exec_read_rdma_addr_1_fixed() { 
        meta.addr1_fixed = read_rdma_addr_1_fixed.execute(0); 
    }

    Register<sum_high_operator, bit<1>>(1) rdma_addr_1; 
    RegisterAction<sum_high_operator, bit<1>, bit<32>>(rdma_addr_1) read_addr_1 = {
        void apply(inout sum_high_operator op, out bit<32> result) { 
            if(op.old_low > meta.addr2) { 
                op.high_sum = op.high_sum + 1; 
            } 
            op.old_low = meta.addr2;
            result = op.high_sum;
        } 
    }; 
    Register<bit<32>, bit<1>>(1) rdma_addr_2; 
    RegisterAction<bit<32>, bit<1>, bit<32>>(rdma_addr_2) read_addr_2 = {
        void apply(inout bit<32> register_data, out bit<32> result) { 
            //register_data = register_data;            
            register_data = register_data + RDMA_PAYLOAD_SIZE;            
            result = register_data;                                 
        } 
    }; 
    action exec_read_addr_1() { 
        meta.addr1 = read_addr_1.execute(0); 
    }
    action exec_read_addr_2() { 
        meta.addr2 = read_addr_2.execute(0); 
    }

    Register<bit<32>, bit<1>>(1) rdma_qpn; 
    RegisterAction<bit<32>, bit<1>, bit<32>>(rdma_qpn) read_qpn = {
        void apply(inout bit<32> register_data, out bit<32> result) { 
            result = register_data;                                 
        } 
    }; 
    action exec_read_qpn() { 
        meta.qpn = read_qpn.execute(0); 
    }
    Register<bit<32>, bit<1>>(1) rdma_rkey; 
    RegisterAction<bit<32>, bit<1>, bit<32>>(rdma_rkey) read_rkey = {
        void apply(inout bit<32> register_data, out bit<32> result) { 
            result = register_data;                                 
        } 
    }; 
    action exec_read_rkey() { 
        meta.rkey = read_rkey.execute(0); 
    }
    Register<bit<32>, bit<1>>(1) rdma_ip; 
    RegisterAction<bit<32>, bit<1>, bit<32>>(rdma_ip) read_ip = {
        void apply(inout bit<32> register_data, out bit<32> result) { 
            result = register_data;                                 
        } 
    }; 
    action exec_read_ip() { 
        meta.ip = read_ip.execute(0); 
    }

    Register<bit<32>, bit<1>>(1) rdma_sqn; 
    RegisterAction<bit<32>, bit<1>, bit<32>>(rdma_sqn) read_update_sqn = {
        void apply(inout bit<32> register_data, out bit<32> result) { 
            result = register_data;                                 
            register_data = register_data + 1;                      
        } 
    }; 
    action exec_read_update_sqn() { 
        meta.sqn = read_update_sqn.execute(0); 
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

    Register<bit<32>, bit<1>>(1) report_count; 
    RegisterAction<bit<32>, bit<1>, bit<32>>(report_count) update_report_count = { 
        void apply(inout bit<32> register_data, out bit<32> result) { 
            //if(register_data == 1048575) {
            if(register_data == 33554432) { //16777216) {
                register_data = 0;
            } else {
                register_data = register_data+ 1; 
            }
            result = register_data;      
        } 
    }; 
    action exec_update_report_count() { 
        meta.report_count = update_report_count.execute(0); 
    }

    RegisterAction<sum_high_operator, bit<1>, bit<32>>(rdma_addr_1) reset_addr_1 = {
        void apply(inout sum_high_operator op, out bit<32> result) { 
            op.high_sum = meta.addr1_fixed;
            op.old_low = 0; 
        } 
    }; 
    RegisterAction<bit<32>, bit<1>, bit<32>>(rdma_addr_2) reset_addr_2 = {
        void apply(inout bit<32> register_data, out bit<32> result) { 
            register_data = meta.addr2_fixed;            
        } 
    }; 
    action exec_reset_addr_1() { 
        reset_addr_1.execute(0); 
    }
    action exec_reset_addr_2() { 
        reset_addr_2.execute(0); 
    }
    bit<1> time_period_expired;
    Register<bit<32>, bit<32>>(1) last_period_timestamp; 
        RegisterAction<bit<32>, bit<1>, bit<1>>(last_period_timestamp) update_last_report_timestamp = {
        void apply(inout bit<32> register_data, out bit<1> result) {
                if (register_data == 0) {
                    register_data = ig_intr_md.ingress_mac_tstamp[41:10];
                    result = 1;
                } else {
                bit<32> tmp;
                tmp = ig_intr_md.ingress_mac_tstamp[41:10] - register_data;
                if(tmp > 20) {
                    register_data = ig_intr_md.ingress_mac_tstamp[41:10];
                    result = 1;
                } else {
                    result = 0;
                }
            }
        }
    };
    action apply_update_last_report_timestamp() {
        meta.time_period_expired = update_last_report_timestamp.execute(0);
    }

  

    apply{
        if(hdr.bridged_md.pkt_type_tcp == PKT_TYPE_ACK || hdr.bridged_md.pkt_type_tcp == PKT_TYPE_FIN_RST) {

        exec_read_rdma_addr_2_fixed();
        exec_read_rdma_addr_1_fixed();

        exec_read_qpn(); 
        exec_read_rkey(); 
        exec_read_ip();

            exec_update_report_count(); 
            if(meta.report_count == 0) {
                exec_reset_addr_1();
                exec_reset_addr_2();
                hdr.bridged_md.resetting = 1;
            } else{
                exec_read_addr_2(); 
                exec_read_addr_1(); 
                
                hdr.ipv4.flags = 0x2;
                hdr.tcp.setInvalid(); hdr.udp.setValid();  hdr.ipv4.total_len=IP_LENGTH; hdr.ipv4.protocol = 17; hdr.rdma_bth.setValid(); hdr.rdma_eth.setValid(); hdr.rdma_payload.setValid(); 
                hdr.ethernet.srcAddr=RDMA_PACKET_MAC; hdr.ethernet.dstAddr=COLLECTOR_MAC; hdr.udp.len=UDP_LENGTH;
                hdr.udp.dst_port = RDMA_PACKET_DST_PORT; hdr.udp.src_port = RDMA_PACKET_SRC_PORT;
                //hdr.rdma_bth.opcode = RDMA_WRITE_OPCODE; hdr.rdma_bth.solicited = 0; hdr.rdma_bth.migreq=1; hdr.rdma_bth.pkey=0xffff; hdr.rdma_bth.ack_request=1;hdr.rdma_bth.dqpn=meta.qpn[23:0]; hdr.rdma_bth.padcount=0; hdr.rdma_bth.psn=meta.sqn[23:0]; hdr.rdma_bth.version=0; hdr.rdma_bth.fecn=0; hdr.rdma_bth.becn=0; hdr.rdma_bth.resv6=0; hdr.rdma_bth.resv7=0;
                hdr.rdma_bth.opcode = 0x2a; hdr.rdma_bth.solicited = 0; hdr.rdma_bth.migreq=1; hdr.rdma_bth.pkey=0xffff; hdr.rdma_bth.ack_request=0;hdr.rdma_bth.dqpn=meta.qpn[23:0]; hdr.rdma_bth.padcount=0; hdr.rdma_bth.version=0; hdr.rdma_bth.fecn=0; hdr.rdma_bth.becn=0; hdr.rdma_bth.resv6=0; hdr.rdma_bth.resv7=0;
                hdr.rdma_eth.addr1=meta.addr1; hdr.rdma_eth.addr2=meta.addr2; hdr.rdma_eth.rkey=meta.rkey; hdr.rdma_eth.dma_len=RDMA_PAYLOAD_SIZE;  
                //hdr.rdma_eth.addr1=0; hdr.rdma_eth.addr2=0; hdr.rdma_eth.rkey=meta.rkey; hdr.rdma_eth.dma_len=RDMA_PAYLOAD_SIZE;  
                hdr.rdma_payload.src_ip = hdr.ipv4.dst_addr; 
                hdr.rdma_payload.dst_ip = hdr.ipv4.src_addr; 
                hdr.ipv4.dst_addr=meta.ip; 
                hdr.ipv4.src_addr=RDMA_PACKET_SRC_IP; 
                hdr.rdma_payload.src_port = hdr.tcp.dst_port; 
                hdr.rdma_payload.dst_port = hdr.tcp.src_port; 
                hdr.rdma_payload.bytes_sketch0 = meta.bytes_sketch0; 
                hdr.rdma_payload.bytes_sketch1 = meta.bytes_sketch1; 
                hdr.rdma_payload.bytes_sketch2 = meta.bytes_sketch2; 
                hdr.rdma_payload.bytes_sketch3 = meta.bytes_sketch3; 
                hdr.rdma_payload.pkt_type = (bit<32>)hdr.bridged_md.pkt_type_tcp; 
                hdr.rdma_payload.pkt_count_sketch0 = meta.pkts_sketch0; 
                hdr.rdma_payload.pkt_count_sketch1 = meta.pkts_sketch1; 
                hdr.rdma_payload.pkt_count_sketch2 = meta.pkts_sketch2; 
                hdr.rdma_payload.pkt_count_sketch3 = meta.pkts_sketch3; 
                hdr.rdma_payload.ack_no = hdr.tcp.ack_no;
                hdr.rdma_payload.timestamp[47:0] = ig_intr_md.ingress_mac_tstamp; 
            }
        if (ig_intr_md.resubmit_flag == 0) { 
            ig_tm_md.ucast_egress_port=152; 
            hdr.bridged_md.do_egr_mirroring = 1; 
            hdr.bridged_md.egr_mir_ses = 1; 
        }
        //SEND_TO(CPU_PORT)
        }
        // SEND_TO(160)
       // }
       // }
    }

}