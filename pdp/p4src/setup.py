from ipaddress import ip_address
from rdma_utils import get_rdma_parameters
import ipaddress


p4 = bfrt.p4rdma.pipe


# This function can clear all the tables and later on other fixed objects
# once bfrt support is added.
def clear_all(verbose=True, batching=True):
    global p4
    global bfrt
    
    # The order is important. We do want to clear from the top, i.e.
    # delete objects that use other objects, e.g. table entries use
    # selector groups and selector groups use action profile members

    for table_types in (['MATCH_DIRECT', 'MATCH_INDIRECT_SELECTOR'],
                        ['SELECTOR'],
                        ['ACTION_PROFILE']):
        for table in p4.info(return_info=True, print_info=False):
            if table['type'] in table_types:
                if verbose:
                    print("Clearing table {:<40} ... ".
                          format(table['full_name']), end='', flush=True)
                table['node'].clear(batch=batching)
                if verbose:
                    print('Done')
                    



def rdma_reset_addr(dev_id, pipe_id, direction, parser_id, session, msg):
    global p4
    global leftmost_32_bits
    global rightmost_32_bits
    print('rdma_reset_addr')
    p4.Ingress.rdma_addr_1.mod(REGISTER_INDEX=0, high_sum=int(leftmost_32_bits, 2))
    p4.Ingress.rdma_addr_2.mod(REGISTER_INDEX=0, f1=int(rightmost_32_bits, 2))
    p4.Ingress.report_count.mod(REGISTER_INDEX=0, f1=0)
    return 0


import time
i = 0
from datetime import datetime

import threading


def reset_sketches():
    global p4
    import time
    while (True):
        time.sleep(10)
        try:
            
            p4.Ingress.rtt_handshake.syns.clear()
            p4.Ingress.rtt_handshake.rtts.clear()
            p4.Ingress.rtt_handshake.clear()


            p4.sketch0.clear()
            p4.sketch1.clear()
            p4.sketch2.clear()
            p4.sketch3.clear()
            
            p4.sketch_packets0.clear()
            p4.sketch_packets1.clear()
            p4.sketch_packets2.clear()
            p4.sketch_packets3.clear()

            print('Bytes sketches resest')
        except Exception as e:
            print ("Error resetting: ", str(e)) 
periodic_reset = threading.Thread(target=reset_sketches, name="reset_sketches", daemon=True)
periodic_reset.start()




# Set RDMA parameters
output, SERVER_IP = get_rdma_parameters()
values = [int(pair.split('=')[1].strip(), 16) for pair in output.split(',')]
QPN, ADDR, RKEY = values
RDMA_PAYLOAD_SIZE = 64


p4.Ingress.rtt_handshake.syns.clear()
p4.Ingress.rtt_handshake.rtts.clear()
p4.Ingress.rtt_handshake.clear()


print(output)
ADDR=int(ADDR)
binary_str = f"{ADDR:064b}"
leftmost_32_bits = binary_str[:32]
rightmost_32_bits = binary_str[-32:]

print('QPN: ', QPN)
print('ADDR: ', ADDR)
print('binary_str: ', f"{ADDR:064b}")
print('RKEY: ', RKEY)


bfrt.mirror.cfg.entry_with_normal(sid=1, direction='EGRESS', session_enable=True, ucast_egress_port=160, ucast_egress_port_valid=1, max_pkt_len=82 + (RDMA_PAYLOAD_SIZE)).push()
#bfrt.mirror.cfg.entry_with_normal(sid=1, direction='EGRESS', session_enable=True, ucast_egress_port=160, ucast_egress_port_valid=1, max_pkt_len=82 + (RDMA_PAYLOAD_SIZE)).push()


#p4.rdma_psn.add(REGISTER_INDEX=0, f1=PSN)
#p4.rdma_msn.add(REGISTER_INDEX=0, f1=0)
#p4.retransmitted_rdma.add(REGISTER_INDEX=0, f1=0)

try:
    # Loop over all combinations of ihl and data_offset
    for ihl in range(5, 16):  # ihl can be 5 to 15
        for data_offset in range(5, 16):  # data_offset can be 5 to 15
            # Calculate the IP and TCP header lengths in bytes
            ip_header_bytes = ihl * 4
            tcp_header_bytes = data_offset * 4
            
            # Assume a minimum IP total length of 20 bytes (ihl=5 and data_offset=5) and no options
            segment_bytes = ip_header_bytes + tcp_header_bytes
            
            # Generate the P4 table add command
            p4.Ingress.total_bytes_logic.tb_calculate_bytes.add_with_set_bytes(data_offset=data_offset, ihl=ihl, headers_bytes=segment_bytes)
except:
    pass

p4.Ingress.rdma.rdma_addr_1.add(REGISTER_INDEX=0, high_sum=int(leftmost_32_bits, 2))
p4.Ingress.rdma.rdma_addr_1_fixed.add(REGISTER_INDEX=0, f1=int(leftmost_32_bits, 2))
p4.Ingress.rdma.rdma_addr_2_fixed.add(REGISTER_INDEX=0, f1=int(rightmost_32_bits, 2))
p4.Ingress.rdma.rdma_addr_2.add(REGISTER_INDEX=0, f1=int(rightmost_32_bits, 2))
p4.Ingress.rdma.rdma_qpn.add(REGISTER_INDEX=0, f1=QPN)
p4.Ingress.rdma.rdma_rkey.add(REGISTER_INDEX=0, f1=RKEY)
p4.Ingress.rdma.rdma_ip.add(REGISTER_INDEX=0, f1=int(ipaddress.ip_address(SERVER_IP)))

