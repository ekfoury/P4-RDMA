control MirrorIngress(inout headers_t hdr, inout metadata_t meta, in ingress_intrinsic_metadata_t ig_intr_md, inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {
    apply {
        if (ig_intr_md.resubmit_flag == 0) { 
            ig_tm_md.ucast_egress_port=136; 
            hdr.bridged_md.do_egr_mirroring = 1; 
            hdr.bridged_md.egr_mir_ses = 1; 
        }
    }
}
