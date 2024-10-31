/*********************  D E P A R S E R  ************************/
control IngressDeparser(packet_out pkt,
    /* User */
    inout headers_t                       hdr,
    in    metadata_t                      meta,
    /* Intrinsic */
    in    ingress_intrinsic_metadata_for_deparser_t  ig_dprsr_md)
{
    Digest<flow_location_digest_t>() flow_location_digest; 
    Digest<bytes_digest_t>() bytes_digest; 
    Digest<ack_count_digest_t>() ack_count_digest; 
    Checksum() ipv4_checksum;

    apply {
        
        if(ig_dprsr_md.digest_type == 1) {

            flow_location_digest.pack({
                                      //Flow ID  
                                      meta.index_sketch,
                                      meta.reverse_index_sketch,
                                      meta.index_sketch0,
                                      meta.index_sketch1,
                                      meta.index_sketch2,
                                      meta.index_sketch3
									  });

            /*
            ack_count_digest.pack({meta.ack_count_sketch0,
                                meta.ack_count_sketch1,
                                meta.ack_count_sketch2,
                                meta.ack_count_sketch3
            });
            */
        }
        

            /*
            
        } 
        */
        
         hdr.ipv4.hdr_checksum = ipv4_checksum.update(
        {hdr.ipv4.version,
            hdr.ipv4.ihl,
            hdr.ipv4.tos,
            hdr.ipv4.total_len,
            hdr.ipv4.identification,
            hdr.ipv4.flags,
            hdr.ipv4.frag_offset,
            hdr.ipv4.ttl,
            hdr.ipv4.protocol,
            hdr.ipv4.src_addr,
            hdr.ipv4.dst_addr});
        pkt.emit(hdr);
    }
}
