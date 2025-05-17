#ifndef JOB_H
#define JOB_H
#include "config.h"
#include <stdbool.h>

typedef enum op {
    FULL,
    ADDED,
    MODIFIED,
    DELETED,
    INVALID_OP
} OPERATION;

typedef struct job {
    char sd[MAX_PATH_SIZE];
    char td[MAX_PATH_SIZE];
    char fn[MAX_FILENAME_SIZE];
    OPERATION op;
    bool valid;
    bool tracked;

    
} Job;

Job create_job(char *src,char *dst ,char *filename,OPERATION op);
void print_job(Job job);
Job default_job();
#endif
