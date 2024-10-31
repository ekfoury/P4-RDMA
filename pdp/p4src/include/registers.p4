#ifndef _REGISTERS_
#define _REGISTERS_

Register<bit<16>, bit<16>>(65536) count_ack;
Register<paired_32bit,bit<16>>(32w65536) reg_table_1;
Register<bit<32>, bit<18>>(131072) total_retr;
Register<bit<32>, bit<18>>(131072) total_sent;
Register<bit<32>, bit<18>>(131072) total_bytes;
Register<bit<32>, bit<1>>(1) rdma_msn; 
Register<bit<32>, bit<1>>(1) rdma_psn; 
Register<bit<8>, bit<1>>(1) rdma_unknown_opcode; 
Register<bit<32>, bit<1>>(1) rdma_psn_error; 
Register<bit<32>, bit<1>>(1) retransmitted_rdma; 

#define SKETCH_REGISTER(num) Register<bit<SKETCH_CELL_BIT_WIDTH>, bit<16>>(SKETCH_BUCKET_LENGTH) sketch##num; 
SKETCH_REGISTER(0)
SKETCH_REGISTER(1)
SKETCH_REGISTER(2)
SKETCH_REGISTER(3)


#define SKETCH_REGISTER_PACKETS(num) Register<bit<SKETCH_CELL_BIT_WIDTH_PACKETS>, bit<16>>(SKETCH_BUCKET_LENGTH) sketch_packets##num; 
SKETCH_REGISTER_PACKETS(0)
SKETCH_REGISTER_PACKETS(1)
SKETCH_REGISTER_PACKETS(2)
SKETCH_REGISTER_PACKETS(3)


#define POLY(num, poly) CRCPolynomial<hash_size_t>(poly, true, false, false, 16w0xFFFF, 16w0xFFFF) crc16d_##num; 
POLY(0, 0x04C11DB7)
POLY(1, 0xEDB88320)
POLY(2, 0x1A83398B)
POLY(3, 0xabc14281)

#define POLY2(num, poly) CRCPolynomial<bit<17>>(poly, true, false, false, 17w0xFFFF, 17w0xFFFF) crc17d_##num; 
POLY2(0, 0x04C11DB7)
POLY2(1, 0xEDB88320)
POLY2(2, 0x1A83398B)
POLY2(3, 0xabc14281)




#endif