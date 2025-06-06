#include "sync_mem.h"
#include <stdio.h>
# include <stdlib.h>
#include <string.h>

typedef struct sync_info_mem_store {
 SyncEntry* array;
 size_t entries;
 size_t max_sz;   
}sync_info_mem_store;

/*
 * djb2 Hash function
 * Original author: Daniel J. Bernstein (http://www.cse.yorku.ca/~oz/hash.html)
 * This function is widely used and is known for its simplicity and speed.
 *
 * License: Public domain (original code by Daniel J. Bernstein)
 */
unsigned long hash(unsigned char *str) {
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

void set_to_default(sync_info_mem_store* sm) {
    SyncEntry def = default_entry();
    for(int i=0;i < sm->max_sz ;i++) {
        sm->array[i] = def;
    }
}


unsigned long linear_probe(SyncEntry* entries,char* key,size_t sz) {
    unsigned long index;
    char* entry_key; 

    index = hash((unsigned char*) key)%(sz);

    for(int i=0;i<sz;i++) {
        entry_key = entries[index].src.;
        if((!entries[index].valid)||(strcmp(key,entry_key)==0)) {
            break;
        }
        index = (index+1)%sz;
    }

    return index;

}

void re_hash(sync_info_mem_store* sm) {
    unsigned long index;
    size_t new_sz = sm->max_sz*2;
    SyncEntry* array = malloc(new_sz*sizeof(SyncEntry));
    SyncEntry def = default_entry();

    for(int i=0;i < new_sz ;i++) {
        array[i] = def;
    }
    
    for(int i=0;i < sm->max_sz ;i++) {
        if((sm->array[i].valid)) {
            index = linear_probe(array,sm->array[i].sd,new_sz);
            array[index] = sm->array[i];
        }
    }

    free(sm->array);
    sm->array = array;
    sm->max_sz = new_sz;
}

void sm_iterate(SyncMem sm,void(*callback)(SyncEntry,char*)) {
    for(int i = 0; i < sm->max_sz; i++) {
        if(sm->array[i].valid) {
            callback(sm->array[i],sm->array[i].sd);
        }
    }
}


int sm_is_empty(sync_info_mem_store* sm) {
    return (sm->entries==0);
}

sync_info_mem_store* sm_create() {
    sync_info_mem_store* sm;
    sm = malloc(sizeof(sync_info_mem_store));
    sm->entries = 0;
    sm->max_sz = DEFAULT_HT_SIZE;
    sm->array = malloc((DEFAULT_HT_SIZE*sizeof(SyncEntry)));
    set_to_default(sm);
    return sm;
}

int sm_add_entry(sync_info_mem_store* sm,SyncEntry entry) {
    unsigned long index;

    if(sm->max_sz == sm->entries) {
        re_hash(sm);
    }
    
    index = linear_probe(sm->array,entry.sd,sm->max_sz);
    sm->array[index] = entry;
    sm->entries++;
    return 0;
}

int sm_del(sync_info_mem_store* sm) {
    free(sm->array);
    free(sm);
    return 0;
}

int get_index(sync_info_mem_store* sm,char* key,unsigned long* out) {
    unsigned long index = hash((char unsigned*) key)%(sm->max_sz);
    for(int i=0;i<(sm->max_sz);i++) {
        index = index%(sm->max_sz);
        if(sm->array[index].valid &&!strcmp(sm->array[index].sd,key)) { 
            *out = index;
            return 0;
        }
        index++;
    }
    return -1;
}

int sm_get_entry(sync_info_mem_store *sm,SyncEntry *out,char *key) {
    unsigned long index;
    if(!get_index(sm,key,&index)) {
        *out = sm->array[index];
        return 0;
    }
    *out = default_entry();
    return -1;
}

int sm_remove_entry(sync_info_mem_store* sm,char* key) {
    unsigned long index;
    if(!get_index(sm,key,&index)) {
        sm->array[index] = default_entry();
        return 0;
    }
    return -1;
}

int sm_search_wd(SyncMem sm,int wd,SyncEntry* out) {
    for(int i = 0; i < sm->max_sz; i++) {
        if((sm->array[i].valid) && (sm->array[i].wd == wd)) {
            *out = sm->array[i];
            return 0;
        }
    }   

    return -1;
}


