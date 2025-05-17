#ifndef UTILS_H
#define UTILS_H
#include <unistd.h>
#include <time.h>

/* Writes a timestamp in ```buff```. Size of buff is ```size```.
    If ```tm``` is -1 current time is used instead.*/
int get_timestamp(char *buff,size_t size,time_t tm);

/* Reads a ```req_size``` chunk from ```fd``` into buffer
    Returns size read ,-1 on error.*/
ssize_t read_buff(char* buffer,ssize_t req_size,int fd);

/* Write a ```req_size``` chunk from buffer into fd/
    Returns size written , -1 on error.*/

ssize_t write_buff(char* buff,ssize_t req_size,int fd);
/* Receive a messsage from ```pipe_in```in ```buff```.
    Returns size received, -1 on error.*/
ssize_t receive_msg(int pipe_in,char* buff);

/* Send a messsage stored in ```buff``` through ```pipe_out```.
    Returns -1 on error.*/
int send_msg(char *msg,ssize_t size,int pipe_out);

#endif