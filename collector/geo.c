



#include <stdio.h>
#include <maxminddb.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <IP address>\n", argv[0]);
        return 1;
    }

    const char *db_path = "/home/ekfoury/digital_twin/collector/collector_c/GeoLite2-City.mmdb";  // Path to GeoLite2 Country database
    const char *ip_address = argv[1];

    MMDB_s mmdb;
    int status = MMDB_open(db_path, MMDB_MODE_MMAP, &mmdb);

    if (status != MMDB_SUCCESS) {
        printf("Error opening database: %s\n", MMDB_strerror(status));
        return 1;
    }

    int gai_error, mmdb_error;
    MMDB_lookup_result_s result = MMDB_lookup_string(&mmdb, ip_address, &gai_error, &mmdb_error);

    if (gai_error != 0) {
        printf("Error from getaddrinfo for IP address %s: %s\n", ip_address, gai_strerror(gai_error));
        return 1;
    }

    if (mmdb_error != MMDB_SUCCESS) {
        printf("Error from libmaxminddb: %s\n", MMDB_strerror(mmdb_error));
        return 1;
    }

    if (result.found_entry) {
        MMDB_entry_data_s entry_data;

        // Look up the latitude
        int lookup_status_lat = MMDB_get_value(&result.entry, &entry_data, "location", "latitude", NULL);
        double latitude = 0.0;
        if (lookup_status_lat == MMDB_SUCCESS && entry_data.has_data && entry_data.type == MMDB_DATA_TYPE_DOUBLE) {
            latitude = entry_data.double_value;
        } else {
            printf("Latitude not found for IP: %s\n", ip_address);
        }

        // Look up the longitude
        int lookup_status_lon = MMDB_get_value(&result.entry, &entry_data, "location", "longitude", NULL);
        double longitude = 0.0;
        if (lookup_status_lon == MMDB_SUCCESS && entry_data.has_data && entry_data.type == MMDB_DATA_TYPE_DOUBLE) {
            longitude = entry_data.double_value;
        } else {
            printf("Longitude not found for IP: %s\n", ip_address);
        }

        printf("Latitude: %f, Longitude: %f\n", latitude, longitude);
    } else {
        printf("No entry for IP address %s in the database.\n", ip_address);
    }

    MMDB_close(&mmdb);
    return 0;
}
