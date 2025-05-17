#include <stdio.h>
#include "worker.h"
#include "string.h"
#include "config.h"
#include "job.h"
#include <stdbool.h>

Worker create_worker(pid_t pid,int fd,Job job) {
    Worker worker;
    worker.pid = pid;
    worker.fd = fd;
    worker.tracked = false;
    worker.assigned_job = job;

    return worker;
}

void print_worker(Worker worker) {
    printf("Worker [pid: %d fd:%d]"
        ,worker.pid,worker.fd);
    print_job(worker.assigned_job);
}
