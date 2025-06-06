#ifndef UTILS_H
#define UTILS_H
#include <unistd.h>
#include <time.h>

typedef struct resource_id{
    char dir[MAX_PATH_SIZE];
    char host[MAX_HOST_SIZE];
    char port[MAX_PORT_SIZE];
} resource_id;

/* Writes a timestamp in ```buff```. Size of buff is ```size```.
    If ```tm``` is -1 current time is used instead.*/
int get_timestamp(char *buff,size_t size,time_t tm);

/* Reads a ```req_size``` chunk from ```fd``` into buffer
    Returns size read ,-1 on error.*/
ssize_t read_buff(char* buffer,size_t req_size,int fd);

/* Write a ```req_size``` chunk from buffer into fd/
    Returns size written , -1 on error.*/

ssize_t write_buff(char* buff,size_t req_size,int fd);
/* Receive a messsage from ```pipe_in```in ```buff```.
    Returns size received, -1 on error.*/
ssize_t receive_msg(int pipe_in,char* buff);

/* Send a messsage stored in ```buff``` through ```pipe_out```.
    Returns -1 on error.*/
ssize_t send_msg(char *msg,size_t size,int pipe_out);

/* Parses a unique resource identified ```uri``` from the formatted ```in``` string.
    Returns -1 on error.*/
int parse_uri(char* in, resource_id *uri);
#endif