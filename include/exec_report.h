#pragma once
#include <sys/types.h>
#include <unistd.h>

typedef enum exec_stat{
    SUCCESS,
    ERROR,
} EXEC_STATUS;

typedef struct exec_report {
    EXEC_STATUS status;
    ssize_t pulled;
    ssize_t pushed;
    char err_msg[PACKET_SIZE];
} Report;