#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "job.h"
#include "queue.h"

typedef struct node_tag{
    Job node_job;
    struct node_tag* next;
} node;

typedef struct job_queue{
    node* front;
    node* rear;
    size_t size;
} job_queue;

node* new_node(Job new_job,node* next) {
    node* new = malloc(sizeof(node));
    new->node_job = new_job;
    new->next = next;
    return new;
}

job_queue *jq_create() {
    job_queue *new_queue = malloc(sizeof(struct job_queue));
    new_queue->front = NULL;
    new_queue->rear = NULL;
    new_queue->size = 0;
    return new_queue;
}

int jq_in_queue(job_queue *q,char *dir) {
    node* curr;
    Job curr_job;

    curr = q->front;
    while(curr!=NULL) {
        curr_job = curr->node_job;
        if(!strcmp(curr_job.sd,dir)) {
            return 1;
        }
        curr = curr->next;
    }
    return 0;
}

int jq_del(job_queue *q) {
    node* head = q->front;
    node* temp;

    while(head) {
        temp = head->next;
        free(head);
        head = temp;
    }
    free(q);
    return 0;
}

int jq_is_empty(job_queue* q) {
    return !(q->size);
}

int jq_size(job_queue* q) {
    return q->size;
}

int jq_enqueue(job_queue* q,Job job) {
    node* new = new_node(job,NULL);
    if(q->size != 0) {
        q->rear->next = new;
        q->rear = q->rear->next; 
    } else {
        q->front = new;
        q->rear = q->front;
    }
    q->size++;

    return 0;
}


int jq_dequeue(job_queue* q,Job* out) {
    node* temp;

    if (q->size == 1) {
        *out = q->front->node_job;
        free(q->front);
        q->front = NULL;
        q->rear = NULL;
        q->size--;

        return 0;
    } else if (q->size != 0) {
        *out = q->front->node_job;
        temp = q->front->next;
        free(q->front);
        q->front = temp;
        q->size--;
        return 0;
    }

    return -1;
}
