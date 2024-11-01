#ifndef IBV_PINGPONG_H
#define IBV_PINGPONG_H

#include <infiniband/verbs.h>

enum ibv_mtu pp_mtu_to_enum(int mtu);
int pp_get_port_info(struct ibv_context *context, int port,
                     struct ibv_port_attr *attr);
void wire_gid_to_gid(const char *wgid, union ibv_gid *gid);
void gid_to_wire_gid(const union ibv_gid *gid, char wgid[]);

#endif /* IBV_PINGPONG_H */