#include <curl/curl.h>
#include "rdma_payload.h"
#include "influxdb_helper.h"
#include "dict.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h> // For inet_ntoa

// Function to initialize a CURL handle with persistent connection and headers
CURL *initialize_curl() {
    CURL *curl = curl_easy_init();
    if (curl) {
        struct curl_slist *headers = NULL;
        char auth_header[512];
        snprintf(auth_header, sizeof(auth_header), "Authorization: Token %s", INFLUXDB_TOKEN);
        headers = curl_slist_append(headers, auth_header);
        headers = curl_slist_append(headers, "Content-Type: text/plain");

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);  // Enable TCP keep-alive

        return curl;
    }
    return NULL;
}

// Function to write data to InfluxDB
void write_data(CURL *curl, const struct rdma_payload_t *payload) {
    char data[1024];
    snprintf(data, sizeof(data), 
        "rdma,src_ip=%s,dst_ip=%s,src_port=%u,dst_port=%u "
        "rtt=%u,total_sent=%u,total_retr=%u,total_bytes=%u,rdma_timestamp=%u",
        ip_to_string(payload->src_ip), ip_to_string(payload->dst_ip), htobe16(payload->src_port), htobe16(payload->dst_port),
        htonl(payload->rtt), htonl(payload->total_sent), htonl(payload->total_retr), htonl(payload->total_bytes),htonl(payload->timestamp));

    char write_url[256];
    snprintf(write_url, sizeof(write_url), "%s/api/v2/write?org=%s&bucket=%s&precision=ns", INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET);

    curl_easy_setopt(curl, CURLOPT_URL, write_url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    } 
}



