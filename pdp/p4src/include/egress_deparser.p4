/*********************  D E P A R S E R  ************************/

control EgressDeparser(packet_out pkt,
    /* User */
    inout headers_t                       hdr,
    in    metadata_t                      meta,
    /* Intrinsic */
    in    egress_intrinsic_metadata_for_deparser_t  eg_dprsr_md)
{
    Checksum() ipv4_checksum;
    Checksum() tcp_checksum;
    Checksum() udp_checksum;


    Mirror() mirror;
    apply {
                 
        if (eg_dprsr_md.mirror_type == MIRROR_TYPE_E2E) {
            mirror.emit<mirror_h>(meta.egr_mir_ses, {meta.pkt_type_mirror});
        }
        pkt.emit(hdr.ethernet);        
        pkt.emit(hdr.ipv4);        
        pkt.emit(hdr.udp);        
        pkt.emit(hdr.rdma_bth);        
        pkt.emit(hdr.rdma_eth);       
        //pkt.emit(hdr.rdma_immdt);        
        pkt.emit(hdr.rdma_payload);        
        pkt.emit(hdr.crc);        
    }
}
