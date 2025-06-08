#ifndef CLIENT_H
#define CLIENT_H
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include <netinet/in.h>

#define MAX_BACKLOG 10
#define MAX_WORKERS 10

typedef enum client_op {
    LIST,
    PUSH,
    PULL,
    INVLD_OP
} CLIENT_OP;

typedef struct header {
    CLIENT_OP op;
    char path[MAX_PATH_SIZE];
    ssize_t chunk_size;
} header_info;

#endif