#ifndef SYNC_MEM_H
#define SYNC_MEM_H
#include "sync.h"
#define DEFAULT_HT_SIZE 64

typedef struct sync_info_mem_store* SyncMem;

/* Retuns a sync info store mem structure handler*/
SyncMem sm_create();

/* Checks wheter sm is empty.*/
int sm_is_empty(SyncMem sm);

/* Free sm.*/
int sm_del(SyncMem sm);

/* Add entry to sm*/
int sm_add_entry(SyncMem sm,SyncEntry entry);

/* Retrieve entry of src direcotry in entry
    Retuns -1 if not found. */
int sm_get_entry(SyncMem sm,SyncEntry *entry, resource_id *id);

#endif

