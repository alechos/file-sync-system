#ifndef FSS_MANAGER_STRING_H
#define FSS_MANAGER_STRING_H

// Log formating strings
#define ADDED_DIR_STR "[%s] Added file: %s -> %s\n"
#define IN_QUEUE_STR "[%s] Already in queue: %s\n"
#define CANCEL_MON_STR "[%s] Synchronization stopped for %s\n"
#define NOT_MON_STR "[%s] Directory not being synchronized: %s\n"
#define MAN_SHUTDOWN_STR "[%s] Shutting down manager...\n"
#define WORKER_WAIT_STR "[%s] Waiting for all active workers to finish.\n"
#define QUEUE_WAIT_STR "[%s] Processing remaining queued tasks.\n"
#define SHUTDOWN_COMPLETE_STR "[%s] Manager shutdown complete.\n"
#define STAT_INFO_STR "Directory: %s\nTarget: %s\nLast Sync: %s\nErrors: %d\nStatus: %s\n"

#define LOG_ENTRY_STR "[%s] [%s] [%s] [%lu] [%s] [%s] [%s]\n"

#endif