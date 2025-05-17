#ifndef QUEUE_H
#define QUEUE_H
#include "job.h"
#define DEFAULT_Q_SIZE 64

typedef struct job_queue* JobQueue;

/* Returns a handler to a new JobQueue instance.
    Returns NULL on failure.*/
JobQueue jq_create();

/* Checks whether JobQueue is empty*/
int jq_is_empty(JobQueue jobs);

/* Returns JobQueue size.*/
int jq_size(JobQueue jobs);

/* Linear search over jobs in JobQueue.
    Returns 1 if job on ```src``` was found, 0 otherwise.*/
int jq_in_queue(JobQueue jobs,char* src);

/* Frees JobQueue.*/
int jq_del(JobQueue jobs);

/* Enqueues ```job``` in JobQueue. */
int jq_enqueue(JobQueue jobs,Job job);

/* Dequeues ```job``` from JobQueue.
    Returns -1 if job not found in ```jobs```.*/
int jq_dequeue(JobQueue jobs,Job* job);
#endif
