#include <string.h>
#include <stdio.h>
#include "sync.h"
#include <time.h>


SyncEntry create_sync_entry(char* src, char* dst,time_t tm,STATUS st) {
    SyncEntry new;

    if(parse_uri(src,&new.src) == -1) {
        new.valid = false;
        return new;
    }

    if(parse_uri(dst,&new.dst) == -1) {
        new.valid = false;
        return new;
    }

    new.status = st;
    new.sync_timestamp = tm;
    new.error_count = 0;
    new.valid = true;

    return new;
}


int get_status_name(STATUS status,char *buff) {
    switch (status)
    {
    case ACTIVE:
        strcpy(buff,"Active");
        break;
    case STOPPED:
        strcpy(buff,"Stopped");
        break;   
    default:
        strcpy(buff,"Error");
        break;
    }

    return 0;
}


void print_sync_entry(SyncEntry en) {
    if(!en.valid) {
        printf("Invalid entry!\n");
        return;
    }
    printf("%s %s %d %ld %d %d\n",en.src.dir,en.dst.dir,en.status,en.sync_timestamp,en.error_count);
}

SyncEntry default_entry() {
    SyncEntry def;
    def.valid = false;
    return def;
}
