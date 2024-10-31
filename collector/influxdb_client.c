#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#define URL "http://localhost:8086"
#define ORG "USC"
#define BUCKET "digital_twin"
#define TOKEN "P5YHgCCXbXg3h0bKlP1Qsx4OEz2jmam1H8sXikKPo12KSBkGiiGGEAQ0fM9lhtMcmbkGeCw5d-P1Nnw3bi8rdg=="

void write_data() {
    CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;

    // Construct the URL for writing data
    char write_url[256];
    snprintf(write_url, sizeof(write_url), "%s/api/v2/write?org=%s&bucket=%s&precision=s", URL, ORG, BUCKET);

    // Data to be written in Line Protocol format
    const char *data = "mem,host=host1 used_percent=23.43234543";

    // Initialize curl
    curl = curl_easy_init();
    if(curl) {
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
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
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
    write_data();
    //query_data();
    return 0;
}

