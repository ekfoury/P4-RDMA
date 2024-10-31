control FlowHashing(inout headers_t hdr, inout metadata_t meta) {

    #define HASH(num) Hash<hash_size_t>(HashAlgorithm_t.CUSTOM, crc16d_##num) hash##num; \
    action apply_hash_##num() { \
        meta.index_sketch##num = hash##num.get({ \
            hdr.ipv4.src_addr, \
            hdr.ipv4.dst_addr, \
            hdr.ipv4.protocol, \
            hdr.tcp.src_port, \
            hdr.tcp.dst_port \
        }); \
    }

    //CRCPolynomial<bit<17>>(0x04C11DB7, true, false, false, 17w0xFFFF, 17w0xFFFF) crc17d;
    Hash<bit<17>>(HashAlgorithm_t.CRC32) hash; 
    action apply_hash() { 
        meta.index_sketch = hash.get({ 
            hdr.ipv4.src_addr, 
            hdr.ipv4.dst_addr, 
            hdr.ipv4.protocol, 
            hdr.tcp.src_port, 
            hdr.tcp.dst_port 
        }); 
    }

    Hash<bit<17>>(HashAlgorithm_t.CRC32) hash_rev; 
    action apply_hash_reverse() { 
        meta.reverse_index_sketch = hash_rev.get({ 
            hdr.ipv4.dst_addr, 
            hdr.ipv4.src_addr, 
            hdr.ipv4.protocol, 
            hdr.tcp.dst_port, 
            hdr.tcp.src_port 
        }); 
    }

    #define HASH_REVERSE(num) Hash<hash_size_t>(HashAlgorithm_t.CUSTOM, crc16d_##num) hash_reverse##num; \
    action apply_hash_reverse_##num() { \
        meta.reverse_index_sketch##num = hash_reverse##num.get({ \
            hdr.ipv4.dst_addr, \
            hdr.ipv4.src_addr, \
            hdr.ipv4.protocol, \
            hdr.tcp.dst_port, \
            hdr.tcp.src_port \
        }); \
    }

    HASH(0)
    HASH(1)
    HASH(2)
    HASH(3)

    HASH_REVERSE(0)
    HASH_REVERSE(1)
    HASH_REVERSE(2)
    HASH_REVERSE(3)

    apply {
        apply_hash();
        apply_hash_0();
        apply_hash_1();
        apply_hash_2();
        apply_hash_3();
        apply_hash_reverse();
        apply_hash_reverse_0();
        apply_hash_reverse_1();
        apply_hash_reverse_2();
        apply_hash_reverse_3();
    }
}
