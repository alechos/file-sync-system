#ifndef UTILS_H
#define UTILS_H
#include "config.h"
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdarg.h>

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

/* Creates a listening socket on ```port```.
    Returns listening socket, -1 on error .*/
int get_listener(short port);

/* Parses a unique resource identified ```uri``` from the formatted ```in``` string.
    Returns -1 on error.*/
int parse_uri(char* in, resource_id *uri);

/* Compare the host,port and dir fields of 2 uris.
    Returns 1 if ```uri_1``` describes the same dir,host and port as ```uri_2```.
    Returns 0 otherwise. */
int compare_uris(resource_id *uri_1,resource_id *uri_2);

/* Connects to a listening peer identified by ```uri``` and returns the socket connection in ```sock```.
    Returns -1 on error.*/
int connect_peer(resource_id *uri,int *sock);

/* Converts resource_id  ```id``` to a string stored in ```str```.
   If ```fn``` is passed as NULL ignore appending filename.
    Returns -1 on error.*/
int uri_string(resource_id *id,char* fn,char* str);


#endif