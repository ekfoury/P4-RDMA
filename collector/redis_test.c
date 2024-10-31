#include <hiredis/hiredis.h>
#include <stdio.h>
#include <stdlib.h>

#define BATCH_SIZE 100  // Define the batch size

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

    // Batch of data
    for (int i = 0; i < BATCH_SIZE; i++) {
        char key[64], value[64];
        sprintf(key, "k%d", i);
        sprintf(value, "value%d", i);

        // Append the SET command to the Redis context buffer
        redisAppendCommand(context, "SET %s %s", key, value);
    }

    // Flush the pipeline: read responses
    for (int i = 0; i < BATCH_SIZE; i++) {
        redisReply *reply;
        if (redisGetReply(context, (void **)&reply) == REDIS_OK) {
            //printf("SET %d result: %s\n", i, reply->str);
            //freeReplyObject(reply);
        } else {
            printf("Error in pipeline reply\n");
            redisFree(context);
            return EXIT_FAILURE;
        }
    }

    // Cleanup and disconnect
    redisFree(context);
    return EXIT_SUCCESS;
}
