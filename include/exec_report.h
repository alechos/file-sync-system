#pragma once
#include <sys/types.h>
#include <unistd.h>

typedef enum exec_stat{
    SUCCESS,
    ERROR,
    PARTIAL,
} EXEC_STATUS;

typedef struct exec_report {
    EXEC_STATUS status;
    int copied;
    int skipped;
    int deleted;
    int errors;
    size_t err_len;
} Report;