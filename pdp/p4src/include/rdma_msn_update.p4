#include "./registers.p4"
control RDMAMsnUpdate(inout headers_t hdr, inout metadata_t meta) {
    RegisterAction<bit<32>, bit<1>, bit<32>>(rdma_msn) update_msn = {
        void apply(inout bit<32> register_data, out bit<32> result) { 
            register_data = (bit<32>)hdr.rdma_bth.psn + 1;            
        } 
    }; 
    action exec_update_msn() { 
        update_msn.execute(0); 
    }

    RegisterAction<bit<32>, bit<1>, bit<32>>(rdma_msn) read_msn = {
        void apply(inout bit<32> register_data, out bit<32> result) { 
            result = register_data;  
            register_data = register_data + 1;   
        } 
    }; 
    action exec_read_msn() { 
        meta.sqn = read_msn.execute(0); // + 1; 
        
    }


    apply {
        
        //if(hdr.rdma_aeth.isValid()) {
        //    exec_update_msn();
        //} else
        if(hdr.bridged_md.pkt_type_tcp == PKT_TYPE_ACK) {
            exec_read_msn();
        }
    
    }

}