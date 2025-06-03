#ifndef SYNC_H
#define SYNC_H
#include "config.h"
#include <time.h>
#include <stdbool.h>

typedef enum status {
    ACTIVE,
    STOPPED,
    ERR
}STATUS;


typedef struct sync_entry {
    char sd[MAX_PATH_SIZE];
    char sh[MAX_HOST_SIZE];
    char sp[MAX_PORT_SIZE];
    
    char td[MAX_PATH_SIZE];
    char th[MAX_HOST_SIZE];
    char tp[MAX_PORT_SIZE];
    STATUS status;
    time_t sync_timestamp;
    unsigned int error_count;
    bool valid;
} SyncEntry;

/* Returns a dummy entry.*/
SyncEntry default_entry();
/* Returns a new synchronization entry.*/
SyncEntry create_sync_entry(char* src, char* dst,time_t tm,STATUS st);
/* Writes status name as a string in ```status_name```*/
int get_status_name(STATUS status,char* status_name);
/* Print a sync entry*/
void print_sync_entry(SyncEntry);

#endif