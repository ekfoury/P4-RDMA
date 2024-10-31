
#ifndef INFLUXDB_HELPER_H
#define INFLUXDB_HELPER_H

#include <curl/curl.h>
#include "rdma_payload.h"

#define INFLUXDB_URL "http://localhost:8086"
#define INFLUXDB_ORG "USC"
#define INFLUXDB_BUCKET "digital_twin"
#define INFLUXDB_TOKEN "P5YHgCCXbXg3h0bKlP1Qsx4OEz2jmam1H8sXikKPo12KSBkGiiGGEAQ0fM9lhtMcmbkGeCw5d-P1Nnw3bi8rdg=="

// Function to initialize a CURL handle with persistent connection and headers
CURL *initialize_curl();

// Function to write data to InfluxDB
void write_data(CURL *curl, const struct rdma_payload_t *payload);

#endif