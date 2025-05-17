#ifndef SYNC_MEM_H
#define SYNC_MEM_H
#include "sync.h"
#define DEFAULT_HT_SIZE 64

typedef struct sync_info_mem_store* SyncMem;

/* Call function callback for each syncentry in sync_mem*/
void sm_iterate(SyncMem sync_mem,void(*callback)(SyncEntry,char*));

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
int sm_get_entry(SyncMem sm,SyncEntry *entry, char *src);

/* Removes entry of src directory.
    Returns -1 if entry not present.*/
int sm_remove_entry(SyncMem sm, char* src);

/* Linear search for an entry with a matching watch descriptor ```wd```.
    Passes retrieved entry to entry.*/
int sm_search_wd(SyncMem sm,int wd,SyncEntry *entry);
#endif

