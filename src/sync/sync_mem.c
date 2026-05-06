#include "sync_mem.h"
#include <stdio.h>
# include <stdlib.h>
#include <string.h>

typedef struct node_tag{
    SyncEntry entry;
    struct node_tag* next;
} node;

typedef struct sync_info_mem_store {
 node* head;
 node* tail;

 size_t entries;

}sync_info_mem_store;

static node* new_node(SyncEntry new_entry,node* next) {
    node* new = malloc(sizeof(node));
    if (new == NULL) return NULL;
    new->entry = new_entry;
    new->next = next;
    return new;
}


int sm_is_empty(sync_info_mem_store* sm) {
    return (sm->entries==0);
}

sync_info_mem_store* sm_create() {
    sync_info_mem_store* sm;
    sm = malloc(sizeof(sync_info_mem_store));
    if (sm == NULL) return NULL;
    sm->head = NULL;
    sm->tail = NULL;
    sm->entries = 0;
    return sm;
}

int sm_add_entry(sync_info_mem_store* sm,SyncEntry entry) {
    node* new = new_node(entry,NULL);
    
    if (new == NULL) return -1;
    new->entry.valid = 1;
    if(sm->entries != 0) {
        sm->tail->next = new;
        sm->tail = new;
    } else {
        sm->head = new;
        sm->tail = sm->head;
    }
    sm->entries++;

    return 0;
}

int sm_del(sync_info_mem_store* sm) {
    node* head;
    node* temp;

    if(sm->entries == 0) return -1;
    head = sm->head;

    while(head) {
        temp = head->next;
        free(head);
        head = temp;
    }
       
    free(sm);
    return 0;
}


int sm_get_entry(sync_info_mem_store *sm,SyncEntry *out,resource_id *id) {
    node* curr;
    SyncEntry curr_entry;

    curr = sm->head;
    while(curr!=NULL) {
        curr_entry = curr->entry;
        if(compare_uris(&curr_entry.src,id)) {
            *out = curr_entry;
            return 0;
        }
        curr = curr->next;
    }
    *out = default_entry();
    return -1;
}



