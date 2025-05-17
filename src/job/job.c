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
        printf("%s %s %s %d\n",pr_job.sd,pr_job.td,pr_job.fn,pr_job.op);
    }
}   

Job create_job(char* sd,char* td,char* fn,OPERATION op) {
    Job new;

    if (!sd || !td || strlen(sd)>MAX_PATH_SIZE || strlen(td)>MAX_PATH_SIZE) {
        return default_job();
    }

    strncpy(new.sd,sd,MAX_PATH_SIZE);
    strncpy(new.td,td,MAX_PATH_SIZE);

    if(!fn || strlen(sd) > MAX_FILENAME_SIZE) {
        strncpy(new.fn,"ALL",MAX_FILENAME_SIZE);
    } else { 
        strncpy(new.fn,fn,MAX_FILENAME_SIZE);
    }
    new.op = op;
    new.valid = true;
    new.tracked = false;
    return new;
}

