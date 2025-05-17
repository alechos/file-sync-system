#ifndef COMMAND_H
#define COMMAND_H

#include "config.h"

// Command type enum
typedef enum command_type {
    ADD,
    STATE,
    CANCEL,
    SYNC,
    SHUTDOWN,
    INVALID_COM
} COMMAND_TYPE;

// Command struct for console/manager communication 
typedef struct command {
    COMMAND_TYPE com;
    char source[MAX_PATH_SIZE];
    char destination[MAX_PATH_SIZE];
} Command;
#endif