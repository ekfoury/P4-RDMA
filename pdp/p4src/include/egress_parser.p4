/***********************  P A R S E R  **************************/

parser EgressParser(packet_in        pkt,
    /* User */
    out headers_t          hdr,
    out metadata_t         meta,
    /* Intrinsic */
    out egress_intrinsic_metadata_t  eg_intr_md)
{
    /* This is a mandatory state, required by Tofino Architecture */
    state start {
        pkt.extract(eg_intr_md);
        transition parse_metadata;
    }

    state parse_metadata {
        mirror_h mirror_md = pkt.lookahead<mirror_h>();
        transition select(mirror_md.pkt_type) {
            PKT_TYPE_MIRROR : parse_mirror_md;
            PKT_TYPE_NORMAL : parse_bridged_md; //parse_bridged_md;
            default : accept;
        }
    }

    state parse_mirror_md {
        mirror_h mirror_md;
        pkt.extract(mirror_md);
        transition parse_ethernet;
    }


    state parse_bridged_md {
        pkt.extract(hdr.bridged_md);
        transition parse_ethernet;
    }

    state parse_ethernet {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.ether_type) {
            ETHERTYPE_IPV4: parse_ipv4;
            default: accept;
        }
    }
    
    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol){
            17: parse_udp;
            6: parse_tcp;
            default: accept;
        }
    }
    state parse_tcp {
        pkt.extract(hdr.tcp);
        transition accept;
    }

    state parse_udp {
        pkt.extract(hdr.udp);
        transition select(hdr.udp.dst_port){
            4791 : parse_rdma_bth;
            default: accept;
        }
    }
    
    state parse_rdma_bth {
        pkt.extract(hdr.rdma_bth);
        transition select(hdr.rdma_bth.opcode){
            10 : parse_rdma_eth;
            42 : parse_rdma_eth;
            default: accept;
        }
    }
    
    state parse_rdma_eth {
        pkt.extract(hdr.rdma_eth);
        transition parse_payload;
    }
        
    state parse_payload {
        pkt.extract(hdr.rdma_payload);
        transition accept;
    }
    
    

}