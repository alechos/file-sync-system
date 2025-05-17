#include "worker_list.h"
#include <string.h>
#include <stdio.h>
#include <job.h>

WorkerList wl_create(int n) {
    WorkerList list;
    list = malloc(sizeof(worker_list));

    Worker def_worker = {.pid = -1, .fd = -1};
    list->capacity = n;
    list->workers = malloc(sizeof(Worker)*n);
    for (int i = 0; i < n; i++) {
        list->workers[i] = def_worker;
    }
    list->size = 0;
    return list;
}

int wl_add(WorkerList list ,Worker worker) {
    int pos;
    if(list->size != list->capacity) {
        for(pos = 0; pos < list->capacity; pos++) {
            if(list->workers[pos].pid == -1) {
                list->workers[pos] = worker;
                break;
            }
        }
        list->size++;
        return pos;
    }
    return -1;
}
int wl_dir_stat(WorkerList list,char *dir) {
    char* tmp_dir;
    Job old_job;
    if(list->size != 0) {
        for(int pos = 0; pos < list->capacity; pos++) {
            old_job = list->workers[pos].assigned_job;
            tmp_dir = old_job.sd;
            if((list->workers[pos].fd != -1) && !strcmp(dir,tmp_dir)) {
                return 1;
            }
        }
    }
    return 0;
}
int wl_remove(WorkerList list,Worker worker) {
    if(list->size != 0) {
        for(int pos = 0; pos < list->capacity; pos++) {
            if(list->workers[pos].pid == worker.pid && worker.pid != -1) {
                list->workers[pos].pid = -1;
                list->workers[pos].fd = -1;
                list->size--;
                return pos;
            }
        }
    }
    return -1;
}

int wl_get_index(WorkerList list,pid_t pid) {
    if(list->size != 0) {
        for(int pos = 0; pos < list->capacity; pos++) {
            if(list->workers[pos].pid == pid) return pos;
        }
    }
    return -1;
}

int wl_free(WorkerList list) {
    free(list->workers);
    free(list);
    return 0;
}
