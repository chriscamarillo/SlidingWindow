#ifndef COMMON_H
#define COMMON_H
#include <stdio.h>
#include <stdlib.h>

#define DATA_SIZE 512
#define WINDOW_SIZE 5
#define TIMEOUT_MS 70
#define EOF_SIGNAL -1337
#define FIRST_TRANSMISSION 0
#define RETRANSMISSION 1
#define SUCCESS 2

typedef struct {
    int32_t seqN;
    int32_t len;
    char data[DATA_SIZE];
} packet;

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

#endif
