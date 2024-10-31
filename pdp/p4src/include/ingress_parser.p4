/***********************  P A R S E R  **************************/
parser IngressParser(packet_in        pkt,
    /* User */
    out headers_t          hdr,
    out metadata_t         meta,
    /* Intrinsic */
    out ingress_intrinsic_metadata_t  ig_intr_md)
{
    /* This is a mandatory state, required by Tofino Architecture */
    state start {
        pkt.extract(ig_intr_md);
        pkt.advance(PORT_METADATA_SIZE);
        transition parse_ethernet;
    }

#ifdef PARSER_OPT
    @critical
#endif
    state parse_ethernet {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.ether_type) {
            ETHERTYPE_IPV4:  parse_ipv4;
            default: accept;
        }
    }
    
#ifdef PARSER_OPT
    @critical
#endif
    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol){
            6 : parse_tcp;
            17: parse_udp;
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
            17 : parse_rdma_aeth;
            default: accept;
        }
    }

    state parse_rdma_aeth {
        pkt.extract(hdr.rdma_aeth);
        transition accept;
    }

}
