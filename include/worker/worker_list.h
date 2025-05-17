#pragma once
#include "worker.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

typedef struct worker_list {
    Worker* workers;
    size_t size;
    size_t capacity;
} worker_list;

typedef worker_list* WorkerList;

/* Returns new worker list handler of given capaity */
WorkerList wl_create(int capacity);

/*Returns 1 if worker with an assigned job for src is in WorkerList.
  Returns 0 otherwise.*/
int wl_dir_stat(WorkerList wl,char *src);

/* Add worker to WorkerList. 
Returns the insertion position if succesful, -1 on failure*/
int wl_add(WorkerList wl,Worker worker);

/* Removes worker from from worker list, search is based on pid.
Returns the removal position if succesful, -1 on failure*/
int wl_remove(WorkerList wl,Worker worker);

int wl_get_index(WorkerList wl,pid_t pid);

/* Free WorkerList.*/
int wl_free(WorkerList wl);
