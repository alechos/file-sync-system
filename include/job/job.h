#ifndef JOB_H
#define JOB_H
#include "utils.h"
#include "config.h"
#include <stdbool.h>

typedef struct job {

    resource_id src;
    resource_id dst;

    char fn[MAX_FILENAME_SIZE];
    bool valid;
    bool tracked;
    
} Job;

Job create_job(resource_id *src,resource_id *dst ,char *filename);
void print_job(Job job);
Job default_job();
#endif
