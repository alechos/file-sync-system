#include <string.h>
#include <stdio.h>
#include "sync.h"
#include "time.h"

SyncEntry create_sync_entry(char* sd,char* td,time_t tm,STATUS st,int wd,unsigned int errs) {
    if ((!sd) || (!td)||(strlen(sd) > MAX_PATH_SIZE) || (strlen(td) > MAX_PATH_SIZE)) {
        return default_entry();
    }
    SyncEntry new;
    strncpy(new.sd,sd,MAX_PATH_SIZE);
    strncpy(new.td,td,MAX_PATH_SIZE);
    new.status = st;
    new.sync_timestamp = tm;
    new.error_count = errs;
    new.wd = wd;
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
    printf("%s %s %d %ld %d %d\n",en.sd,en.td,en.status,en.sync_timestamp,en.error_count,en.wd);
}

SyncEntry default_entry() {
    SyncEntry def;
    def.valid = false;
    return def;
}
