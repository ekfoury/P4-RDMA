#include "./registers.p4"
control RDMAMsnRead(inout headers_t hdr, inout metadata_t meta) {
    RegisterAction<bit<32>, bit<1>, bit<32>>(rdma_msn) read_msn = {
        void apply(inout bit<32> register_data, out bit<32> result) { 
            result = register_data;     
        } 
    }; 
    action exec_read_msn() { 
            meta.sqn = read_msn.execute(0); // + 1; 
        
    }


    apply {
        exec_read_msn();
    }

}