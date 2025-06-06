#ifndef COMMAND_H
#define COMMAND_H

#include "config.h"

// Command type enum
typedef enum command_type {
    ADD,
    CANCEL,
    SHUTDOWN,
    INVALID_COM
} COMMAND_TYPE;

// Command struct for console/manager communication 
typedef struct command {
    COMMAND_TYPE com;
    char source[MAX_URI_LEN];
    char destination[MAX_URI_LEN];
} Command;
#endif