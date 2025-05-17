#ifndef FSS_MANAGER_STRING_H
#define FSS_MANAGER_STRING_H

// Log formating strings
#define ADDED_DIR_STR "[%s] Added directory: %s -> %s\n"
#define MON_STARTED_STR "[%s] Monitoring started for %s\n"
#define IN_QUEUE_STR "[%s] Already in queue: %s\n"
#define CANCEL_MON_STR "[%s] Monitoring stopped for %s\n"
#define NOT_MON_STR "[%s] Directory not monitored: %s\n"
#define STAT_REQ_STR "[%s] Status requested for %s\n"
#define SYNC_DIR_STR "[%s] Syncing directory: %s -> %s\n"
#define SYNC_DONE_STR "[%s] Sync completed %s -> %s Errors:%d\n"
#define SYNC_PEND_STR "[%s] Sync already in progress %s\n"
#define MAN_SHUTDOWN_STR "[%s] Shutting down manager...\n"
#define WORKER_WAIT_STR "[%s] Waiting for all active workers to finish.\n"
#define QUEUE_WAIT_STR "[%s] Processing remaining queued tasks.\n"
#define SHUTDOWN_COMPLETE_STR "[%s] Manager shutdown complete.\n"
#define STAT_INFO_STR "Directory: %s\nTarget: %s\nLast Sync: %s\nErrors: %d\nStatus: %s\n"

#define LOG_ENTRY_STR "[%s] [%s] [%s] [%d] [%s] [%s] [%s]\n"

#endif