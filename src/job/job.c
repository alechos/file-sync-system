#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "job.h"

Job default_job() {
    Job job;
    job.valid = false;
    return job; 
}

void print_job(Job pr_job) {
    if(!pr_job.valid) {
        printf("Invalid job!");
    } else {
        printf("%s %s %s\n",pr_job.src.dir,pr_job.dst.dir,pr_job.fn);
    }
}   

Job create_job(resource_id *src, resource_id *dst,char *filename) {
    Job new;

    new.src = *src;
    new.dst = *dst;
    strncpy(new.fn,filename,MAX_FILENAME_SIZE);

    new.valid = true;
    new.tracked = false;
    return new;
}

