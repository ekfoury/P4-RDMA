compile:
	gcc -o rdma_server rdma_server3.c rdma_cm_manager.c dict.c top_flows.c ./libgeohash/geohash.c -lrdmacm -libverbs -lncurses -lcurl -lhiredis -lmaxminddb 
run:
	rm -rf CSVs/* & ./rdma_server -g 3 -d mlx5_1
debug:
	gdb ./rdma_server
