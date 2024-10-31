#include "rdma_cm_manager.h"
#include "rdma_payload.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <unistd.h>  // Include this header for sleep function declaration
#include <byteswap.h>
#include <rdma/rdma_cma.h> 
#include <inttypes.h>  // Include inttypes.h for PRIx64 and PRIx32 macros
#include <time.h>

int prepare_recv_notify_before_using_rdma_write(struct rdma_cm_id *cm_id, struct ibv_pd *pd)
{   
    struct rdma_payload_t *buf = calloc(1, RDMA_PAYLOAD_SIZE);

	struct ibv_mr *mr = ibv_reg_mr(pd, buf, RDMA_PAYLOAD_SIZE, IBV_ACCESS_LOCAL_WRITE); 
	if (!mr) 
		return 1;
    struct ibv_sge notify_sge = {
        .addr = (uintptr_t)buf,
        .length = RDMA_PAYLOAD_SIZE,
        .lkey = mr->lkey,
    };
    struct ibv_recv_wr notify_wr = {
        .wr_id = 0,
        .sg_list = &notify_sge,
        .num_sge = 1,
        .next = NULL,
    };
    struct ibv_recv_wr *bad_recv_wr;
    if (ibv_post_recv(cm_id->qp,&notify_wr,&bad_recv_wr))
		return 1;

    return 0;
}


int initialize_rdmacm(struct rdma_payload_t **buf) {
    struct pdata                rep_pdata;
    struct rdma_event_channel   *cm_channel;
    struct rdma_cm_id           *listen_id; 
    struct rdma_cm_id           *cm_id; 
    struct rdma_cm_event        *event; 
    struct rdma_conn_param      conn_param = { };
    struct ibv_pd               *pd; 
    struct ibv_comp_channel     *comp_chan; 
    struct ibv_cq               *cq;
    struct ibv_cq               *evt_cq;
    struct ibv_mr               *mr; 
    struct ibv_qp_init_attr     qp_attr = { };
    struct ibv_sge              sge; 
    struct ibv_send_wr          send_wr = { };
    struct ibv_send_wr          *bad_send_wr; 
    struct ibv_wc               wc;
    void                        *cq_context;
    struct sockaddr_in          sin;
    int                         err;

    cm_channel = rdma_create_event_channel();
    if (!cm_channel) 
        return 1;
    
    err = rdma_create_id(cm_channel,&listen_id,NULL,RDMA_PS_TCP); 
    if (err) 
        return err;
    
    sin.sin_family = AF_INET; 
    sin.sin_port = htons(20000);
    sin.sin_addr.s_addr = INADDR_ANY;

    err = rdma_bind_addr(listen_id,(struct sockaddr *)&sin);
    if (err) {
        printf("Cannot bind address. Is another server already running?");
        return 1;
    }

    err = rdma_listen(listen_id,1);
    if (err)
        return 1;

    err = rdma_get_cm_event(cm_channel,&event);
    if (err)
        return err;
    if (event->event != RDMA_CM_EVENT_CONNECT_REQUEST)
        return 1;
    cm_id = event->id;
    /* Each rdmacm event should be acked. */
    rdma_ack_cm_event(event);

    pd = ibv_alloc_pd(cm_id->verbs);
    if (!pd) 
        return 1;


    cq = ibv_create_cq(cm_id->verbs,8192,NULL,NULL,0); 
    if (!cq)
        return 1;

    *buf = calloc(CIRCULAR_BUFFER_SIZE, RDMA_PAYLOAD_SIZE);
    memset(*buf, 0, (size_t)CIRCULAR_BUFFER_SIZE * RDMA_PAYLOAD_SIZE);
    if (!*buf) {
        perror("Failed to allocate buffer");
        return 1;
    }
    
    /* register a memory region with a specific pd */
    mr = ibv_reg_mr(pd, *buf, (size_t) CIRCULAR_BUFFER_SIZE * RDMA_PAYLOAD_SIZE, 
        IBV_ACCESS_LOCAL_WRITE | 
        IBV_ACCESS_REMOTE_READ | 
        IBV_ACCESS_REMOTE_WRITE); 
    if (!mr) 
        return 1;
    
    memset(&qp_attr,0,sizeof(qp_attr));
    qp_attr.cap.max_send_wr = 8192;;
    qp_attr.cap.max_send_sge = 1;
    qp_attr.cap.max_recv_wr = 8192;;
    qp_attr.cap.max_recv_sge = 1;

    qp_attr.send_cq = cq;
    qp_attr.recv_cq = cq;

    //qp_attr.qp_type = IBV_QPT_UD;
    qp_attr.qp_type = IBV_QPT_RC;

    err = rdma_create_qp(cm_id,pd,&qp_attr); 
    if (err) {
        perror("rdma cm create qp error");
        return err;
    }
    uint32_t qp_num = cm_id->qp->qp_num;

    rep_pdata.buf_va = bswap_64((uintptr_t)*buf); 
    /* we need to prepare remote key to give to client */
    rep_pdata.buf_rkey = htonl(mr->rkey); 
    rep_pdata.qp_num = htonl(qp_num); 
    
    printf("QP: %" PRIx32 "\n", qp_num);
    printf("RKEY:  %" PRIx32 "\n", mr->rkey);
    printf("ADDRESS:  %" PRIx64 "\n", (uintptr_t)*buf);

    conn_param.responder_resources = 16;  
    conn_param.private_data = &rep_pdata; 
    conn_param.private_data_len = sizeof(rep_pdata);

    err = rdma_accept(cm_id,&conn_param); 
    if (err) 
        return 1;
    err = rdma_get_cm_event(cm_channel,&event);
    if (err) 
        return err;
    if (event->event != RDMA_CM_EVENT_ESTABLISHED)
        return 1;
    rdma_ack_cm_event(event);

    for (int i=0; i<8192; i++) {
        prepare_recv_notify_before_using_rdma_write(cm_id, pd);
    }

    return 0;
}



