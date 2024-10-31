control MirrorEgress(inout headers_t hdr, inout metadata_t meta, inout egress_intrinsic_metadata_for_deparser_t eg_dprsr_md) {
    action set_mirror() { 
        meta.egr_mir_ses = hdr.bridged_md.egr_mir_ses; 
        meta.pkt_type_mirror = PKT_TYPE_MIRROR;        
        eg_dprsr_md.mirror_type = MIRROR_TYPE_E2E;     
    }
    apply {
        set_mirror();
    }
}