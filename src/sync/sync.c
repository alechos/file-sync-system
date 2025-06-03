#include <string.h>
#include <stdio.h>
#include "sync.h"
#include "time.h"

int parse_sync_entry(char* in, char* dir, char* host, char* port) {
    char str[MAX_URI_LEN];
    char *start, *end;

    strcpy(str, in);

    start = str;
    end = strchr(str, '@');
    if (end == NULL || start == NULL) return -1;

    *end = '\0';

    snprintf(dir, MAX_PATH_SIZE, "%s", start);

    start = end + 1;
    end = strchr(start, ':');
    if (end == NULL || start == NULL) return -1;

    *end = '\0';
    snprintf(host, MAX_HOST_SIZE, "%s", start);

    start = end + 1;
    if (*start == '\0') return -1;

    snprintf(port, MAX_PORT_SIZE, "%s", start);

    return 0;
}


SyncEntry create_sync_entry(char* src, char* dst,time_t tm,STATUS st) {
    SyncEntry new;

    if(parse_sync_entry(src,&new.sd,&new.sh,&new.sp) == -1) {
        new.valid = false;
        return new;
    }

    if(parse_sync_entry(dst,&new.td,&new.th,&new.tp) == -1) {
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
    printf("%s %s %d %ld %d %d\n",en.sd,en.td,en.status,en.sync_timestamp,en.error_count,en.wd);
}

SyncEntry default_entry() {
    SyncEntry def;
    def.valid = false;
    return def;
}
