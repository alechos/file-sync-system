#ifndef WORKER_H
#define WORKER_H
#include "job.h"
#include "config.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>


typedef struct worker {
    pid_t pid;
    int fd;
    bool tracked;
    Job assigned_job;
} Worker;

/* Create a worker with given pid, pipe fd and worker-assigned job.*/
Worker create_worker(pid_t pid,int pipe,Job assigned_job);
/* Print a worker.*/
void print_worker(Worker worker);
#endif