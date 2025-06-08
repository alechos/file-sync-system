#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "job.h"
#include "queue.h"


typedef struct node_tag{
    Job node_job;
    struct node_tag* next;
} node;

typedef struct job_queue{
    node* front;
    node* rear;
    int shutdown_flag;
    size_t size;
    size_t max_size;

    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} job_queue;

node* new_node(Job new_job,node* next) {
    node* new = malloc(sizeof(node));
    new->node_job = new_job;
    new->next = next;
    return new;
}

job_queue *jq_create(size_t max_size) {

    job_queue *new_queue = malloc(sizeof(struct job_queue));
    new_queue->shutdown_flag = 0;
    new_queue->front = NULL;
    new_queue->rear = NULL;
    new_queue->size = 0;
    new_queue->max_size = max_size;

    pthread_mutex_init(&new_queue->mutex, NULL);
    pthread_cond_init(&new_queue->not_empty, NULL);
    pthread_cond_init(&new_queue->not_full, NULL);
    return new_queue;

}

int jq_in_queue(job_queue *q,Job* job) {
    node* curr;
    Job curr_job;

    pthread_mutex_lock(&q->mutex);

    curr = q->front;
    while(curr!=NULL) {
        curr_job = curr->node_job;
//      WIF : make sure job fields are thread safe
        if(compare_uris(&curr_job.src,&job->src) && !strcmp(curr_job.fn,job->fn)) {
            pthread_mutex_unlock(&q->mutex);
            return 1;
        }
        curr = curr->next;
    }
    pthread_mutex_unlock(&q->mutex);
    return 0;


}

int jq_del(job_queue *q) {
 
    node* head;
    node* temp;

    pthread_mutex_lock(&q->mutex);
    head = q->front;

    while(head) {
        temp = head->next;
        free(head);
        head = temp;
    }
    
    pthread_mutex_unlock(&q->mutex);
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->not_empty); 
    pthread_cond_destroy(&q->not_full);

    free(q);

    return 0;
}

int jq_is_empty(job_queue* q) {
    size_t size;

    pthread_mutex_lock(&q->mutex);
    size = q->size;
    pthread_mutex_unlock(&q->mutex);

    return !size;
}

int jq_size(job_queue* q) {
    size_t size;

    pthread_mutex_lock(&q->mutex);
    size = q->size;
    pthread_mutex_unlock(&q->mutex);

    return size;
}

int jq_enqueue(job_queue* q,Job job) {
    node* new = new_node(job,NULL);

    pthread_mutex_lock(&q->mutex);
    while(q->size >= q->max_size) {
        pthread_cond_wait(&q->not_full,&q->mutex);
    }

    if(q->size != 0) {
        q->rear->next = new;
        q->rear = new; 
    } else {
        q->front = new;
        q->rear = q->front;
    }
    q->size++;
    new->node_job.valid = 1;
    pthread_cond_broadcast(&q->not_empty);
    pthread_mutex_unlock(&q->mutex);

    return 0;
}

int jq_dequeue(job_queue* q,Job* out) {
    node* temp;
    pthread_mutex_lock(&q->mutex);
    while(q->size<=0) {
        pthread_cond_wait(&q->not_empty,&q->mutex);
    }

    *out = q->front->node_job;
    temp = q->front->next;
    free(q->front);
    q->front = temp;
    q->size--;
    if (q->size == 0) q->rear = NULL;

    pthread_cond_broadcast(&q->not_full);
    pthread_mutex_unlock(&q->mutex);

    return 0;
}

int jq_cancel(job_queue* q,char* dir) {
    node* curr;
    Job curr_job;

    pthread_mutex_lock(&q->mutex);

    curr = q->front;
    while(curr!=NULL) {
        curr_job = curr->node_job;
        if(!strcmp(curr_job.src.dir,dir)) {
            curr->node_job.valid = 0;
        }
        curr = curr->next;
    }
    pthread_mutex_unlock(&q->mutex);
    return 0;

}

int jq_shutdown(job_queue* q) {
    pthread_mutex_lock(&q->mutex);
    //drains queue, rechecking everytime a job is removed
    while(q->size>0) {
        pthread_cond_wait(&q->not_full,&q->mutex);
    }
    q->shutdown_flag = 1;
    pthread_mutex_unlock(&q->mutex);
    return 0;
}

int jq_is_shutdown(job_queue* q) {
    int ret;

    pthread_mutex_lock(&q->mutex);
    ret = q->shutdown_flag;
    pthread_mutex_unlock(&q->mutex);
    
    return ret;
}