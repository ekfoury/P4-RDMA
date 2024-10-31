#include <hiredis/hiredis.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define BATCH_SIZE 1000000  // Define the batch size

int main(void) {
    // Connect to Redis server
    redisContext *context = redisConnect("127.0.0.1", 6379);
    if (context == NULL || context->err) {
        if (context) {
            printf("Connection error: %s\n", context->errstr);
            redisFree(context);
        } else {
            printf("Connection error: Can't allocate redis context\n");
        }
        return EXIT_FAILURE;
    }

    // Track time before batch writing
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Batch of data
    for (int i = 0; i < BATCH_SIZE; i++) {
        char key[64], value[64];
        sprintf(key, "b%d", i);
        sprintf(value, "value%d", i);

        // Append the SET command to the Redis context buffer
        redisAppendCommand(context, "SET %s %s", key, value);
    }

    // Flush the pipeline: read responses
    for (int i = 0; i < BATCH_SIZE; i++) {
        redisReply *reply;
        if (redisGetReply(context, (void **)&reply) == REDIS_OK) {
            freeReplyObject(reply);  // Free the reply object
        } else {
            printf("Error in pipeline reply\n");
            redisFree(context);
            return EXIT_FAILURE;
        }
    }

    // Track time after batch writing
    clock_gettime(CLOCK_MONOTONIC, &end);

    // Calculate elapsed time in seconds
    double elapsed_time = (end.tv_sec - start.tv_sec) +
                          (end.tv_nsec - start.tv_nsec) / 1000000000.0;

    // Calculate writes per second
    double writes_per_second = BATCH_SIZE / elapsed_time;
    printf("Writes per second: %.2f\n", writes_per_second);

    // Cleanup and disconnect
    redisFree(context);
    return EXIT_SUCCESS;
}
