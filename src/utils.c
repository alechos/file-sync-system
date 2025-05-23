#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <arpa/inet.h>
#include "utils.h"

int get_timestamp(char *buff,size_t size,time_t now) {
    if(now == -1) now = time(NULL);
    struct tm *lcl_time = localtime(&now);
    strftime(buff,size,"%Y-%m-%d %H:%M:%S",lcl_time);
    return 0;
}

ssize_t read_buff(char* buffer,size_t req_size,int fd) {
    ssize_t total_r,bytes_r;
    char *ptr;

    ptr = buffer;
    total_r = 0;
    bytes_r = 0;
    while((total_r < req_size) && ((bytes_r = read(fd,ptr,req_size - total_r)) > 0)) {
        total_r+=bytes_r;
        ptr+=bytes_r;
    }

    if(bytes_r == -1) {
        return -1;
    }

    return total_r;
}

ssize_t write_buff(char* buff,size_t req_size,int fd) {
    ssize_t bytes_w,total_w;
    char* ptr;

    ptr = buff;
    total_w = 0;
    bytes_w = 0;

    while( ((total_w < req_size)) && (bytes_w  = write(fd,ptr,req_size - total_w)) > 0) {
        total_w+= bytes_w;
        ptr+=bytes_w;
    }

    if (bytes_w == -1) {
        return -1;
    }

    return total_w;
}

//WIF: biff is smaller than message? maybe set a cap from a passsed parameter
ssize_t receive_msg(int sock_in,char* buff) {
    uint32_t net_size; 
    size_t msg_size;
    ssize_t bytes_read;

    if(read_buff((char*)&net_size,sizeof(uint32_t),sock_in) == -1) return -1;
    msg_size = ntohl(net_size);
    if((bytes_read = read_buff(buff,msg_size,sock_in)) == -1) return -1;

    return bytes_read;
}

ssize_t send_msg(char *msg,size_t size,int sock_out) {
    uint32_t net_size = htonl(size);
    ssize_t total_w;

    if(write_buff((char*)&net_size,sizeof(uint32_t),sock_out) == -1) return -1;
    if((total_w = write_buff(msg,size,sock_out)) == -1) return -1;

    return 0;
}