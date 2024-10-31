#include "./registers.p4"

control RTTHandshakeSyn(inout headers_t hdr, inout metadata_t meta) {
    Register<bit<32>, bit<17>>(131072) rtts;
    Register<bit<32>, bit<17>>(131072) syns;

    Register<bit<32>, bit<17>>(131072) rtts2;
    Register<bit<32>, bit<17>>(131072) syns2;

    RegisterAction<bit<32>, bit<17>, bit<32>>(rtts) update_rtt = {
        void apply(inout bit<32> register_data, out bit<32> result) {
            //  if(meta.rtt < 100000) { // && meta.rtt < 3100) {
                register_data = meta.rtt;
            //  } 
        }
    };
     RegisterAction<bit<32>, bit<17>, bit<32>>(rtts) get_rtt = {
        void apply(inout bit<32> register_data, out bit<32> result) {
            result = register_data;
        }
    };
    
    RegisterAction<bit<32>, bit<17>, bit<32>>(syns) read_syn_time = {
        void apply(inout bit<32> register_data, out bit<32> result) {
            //if(meta.time > register_data) {
            if(register_data != 0) {
                result = meta.time - register_data;
            } else {
                result = 0;
            }
        }
    };

    RegisterAction<bit<32>, bit<17>, bit<32>>(syns) update_syn_time = {
        void apply(inout bit<32> register_data, out bit<32> result) {
             bool existing_timestamp_is_old = (meta.time-register_data)>(10000);
             if(existing_timestamp_is_old || register_data == 0) {
                register_data = meta.time;
             }
        }
    };


    





    RegisterAction<bit<32>, bit<17>, bit<32>>(rtts2) update_rtt2 = {
        void apply(inout bit<32> register_data, out bit<32> result) {
            // if(meta.rtt2 < 500000) { // && meta.rtt2 < 3100) {
                register_data = meta.rtt2;
            // } 
        }
    };
     RegisterAction<bit<32>, bit<17>, bit<32>>(rtts2) get_rtt2 = {
        void apply(inout bit<32> register_data, out bit<32> result) {
            result = register_data;
        }
    };
    
    RegisterAction<bit<32>, bit<17>, bit<32>>(syns2) read_syn_time2 = {
        void apply(inout bit<32> register_data, out bit<32> result) {
            //if(meta.time > register_data) {
            // if(meta.time > register_data) {
            if(register_data != 0) {
                result = meta.time - register_data;
            } else {
                result = 0;
            }

                //register_data = 0;
            // } else {
            //     result = 0;
            // }
        }
    };

    RegisterAction<bit<32>, bit<17>, bit<32>>(syns2) update_syn_time2 = {
        void apply(inout bit<32> register_data, out bit<32> result) {
            bool existing_timestamp_is_old = (meta.time-register_data)>(10000);
            if(existing_timestamp_is_old || register_data == 0) {
                register_data = meta.time;
            }
        }
    };



    
    apply {
        
        if (hdr.bridged_md.pkt_type_tcp == PKT_TYPE_SYN) {
            update_syn_time.execute(meta.index_sketch);
            //update_syn_time2.execute(meta.index_sketch);
        } 
        else if (hdr.bridged_md.pkt_type_tcp == PKT_TYPE_SYNACK) {
            meta.rtt = read_syn_time.execute(meta.reverse_index_sketch);
            //meta.rtt2 = read_syn_time2.execute(meta.reverse_index_sketch);
            // if(meta.time > meta.syn_time){
            //      meta.rtt = meta.time - meta.syn_time;
            // }
            // if(meta.time > meta.syn_time){
            //      meta.rtt2 = meta.time - meta.syn_time2;
            // }
            //if(meta.syn_time != 0) {
            
                update_rtt.execute(meta.reverse_index_sketch);
                //update_rtt2.execute(meta.reverse_index_sketch);
            //}
        } else if (hdr.bridged_md.pkt_type_tcp == PKT_TYPE_ACK) {           
            //hdr.rdma_payload.rtt =  get_rtt.execute(meta.index_sketch);
            hdr.rdma_payload.rtt =  get_rtt.execute(meta.reverse_index_sketch);
        }
        
    }
}