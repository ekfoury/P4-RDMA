#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <stdint.h>

#define URL "http://localhost:8086"
#define ORG "USC"
#define BUCKET "digital_twin"
#define TOKEN "P5YHgCCXbXg3h0bKlP1Qsx4OEz2jmam1H8sXikKPo12KSBkGiiGGEAQ0fM9lhtMcmbkGeCw5d-P1Nnw3bi8rdg=="

struct rdma_payload_t {
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t rtt;
    uint32_t total_sent;
    uint32_t total_retr;
    uint32_t total_bytes;
    uint32_t timestamp;
};

void write_data(CURL *curl, const struct rdma_payload_t *payload) {
    CURLcode res;
    struct curl_slist *headers = NULL;
    
    // Construct the URL for writing data
    char write_url[256];
    snprintf(write_url, sizeof(write_url), "%s/api/v2/write?org=%s&bucket=%s&precision=s", URL, ORG, BUCKET);

    // Format the data into Line Protocol format
    char data[512];

    snprintf(data, sizeof(data), 
        "rdma2,src_ip=%u,dst_ip=%u,src_port=%u,dst_port=%u "
        "rtt=%u,total_sent=%u,total_retr=%u,total_bytes=%u,rdma_timestamp=%u",
        payload->src_ip, payload->dst_ip, payload->src_port, payload->dst_port,
        payload->rtt, payload->total_sent, payload->total_retr, payload->total_bytes,payload->timestamp);
														    //
    // Set the URL
    curl_easy_setopt(curl, CURLOPT_URL, write_url);

    // Add the authorization header
    char auth_header[512];
    snprintf(auth_header, sizeof(auth_header), "Authorization: Token %s", TOKEN);
    headers = curl_slist_append(headers, auth_header);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Specify the POST data
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

    // Perform the request
    res = curl_easy_perform(curl);
    if(res != CURLE_OK)
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

    // Clean up
    curl_slist_free_all(headers);
}

void query_data() {
    CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;

    // Construct the URL for querying data
    char query_url[256];
    snprintf(query_url, sizeof(query_url), "%s/api/v2/query?org=%s", URL, ORG);

    // Flux query to be executed
    const char *query = "{\"query\": \"from(bucket: \\\"digital_twin\\\") |> range(start: -1h)\"}";

    // Initialize curl
    curl = curl_easy_init();
    if(curl) {
        // Set the URL
        curl_easy_setopt(curl, CURLOPT_URL, query_url);

        // Add the headers
        char auth_header[512];
        snprintf(auth_header, sizeof(auth_header), "Authorization: Token %s", TOKEN);
        headers = curl_slist_append(headers, auth_header);
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Specify the POST data (Flux query)
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, query);

        // Perform the request
        res = curl_easy_perform(curl);
        if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

        // Clean up
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
}

int main(void) {
    CURL *curl;

    // Initialize curl
    curl = curl_easy_init();
    if(curl) {
        // Example payload
        struct rdma_payload_t payload = {
            .src_ip = 0x7F000001,  // 127.0.0.1
            .dst_ip = 0x7F000001,  // 127.0.0.1
            .src_port = 12345,
            .dst_port = 80,
            .rtt = 10,
            .total_sent = 1000,
            .total_retr = 5,
            .total_bytes = 2048,
            .timestamp = 1692284400  // Example timestamp
        };

        // Write data
        write_data(curl, &payload);

        // Clean up
        curl_easy_cleanup(curl);
    }

    //query_data();
    return 0;
}

