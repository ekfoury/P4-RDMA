#ifndef RDMA_CM_MANAGER_H
#define RDMA_CM_MANAGER_H

#include <rdma/rdma_cma.h>
#include <infiniband/verbs.h>
#include "rdma_payload.h"
#include "constants.h"


struct pdata { 
    uint64_t buf_va; 
    uint32_t buf_rkey; 
    uint32_t qp_num;      
};

int initialize_rdmacm(struct rdma_payload_t **);

#endif // RDMA_CM_MANAGER_H
