#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <arpa/inet.h>
#include "utils.h"
#include "config.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>


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

//WIF regarding the WIF at receive_msg... maybe prohibit sending above packet_size or at least notify of trunct
ssize_t send_msg(char *msg,size_t size,int sock_out) {
    uint32_t net_size = htonl(size);
    ssize_t total_w;

    if(write_buff((char*)&net_size,sizeof(uint32_t),sock_out) == -1) return -1;
    if((total_w = write_buff(msg,size,sock_out)) == -1) return -1;

    return 0;
}

int uri_string(resource_id *id,char* fn,char* str) {
    
    if(fn == NULL) {
        snprintf(str,MAX_URI_LEN,"%s@%s:%s",id->dir,id->host,id->port);
    } else {
        snprintf(str,MAX_URI_LEN,"%s/%s@%s:%s",id->dir,fn,id->host,id->port);
    }

    return 0;
}

int parse_uri(char* in, resource_id *uri) {
    char str[MAX_URI_LEN];
    char *start, *end;

    strcpy(str, in);

    start = str;
    end = strchr(str, '@');
    if (end == NULL || start == NULL) return -1;

    *end = '\0';
    snprintf(uri->dir, MAX_PATH_SIZE, "%s", start);
    start = end + 1;
    end = strchr(start, ':');
    if (end == NULL || start == NULL) return -1;

    *end = '\0';
    snprintf(uri->host, MAX_HOST_SIZE, "%s", start);
    start = end + 1;
    if (*start == '\0') return -1;

    snprintf(uri->port, MAX_PORT_SIZE, "%s", start);

    return 0;
}

int compare_uris(resource_id *uri_1,resource_id *uri_2) {
    return  ((!strcmp(uri_1->dir,uri_2->dir)) && 
            (!strcmp(uri_1->host,uri_2->host)) &&
            (!strcmp(uri_1->port,uri_2->port)));
}

int get_listener(short port) {
    struct sockaddr_in host; 
    int listen_sock,option;

    if((listen_sock = socket(AF_INET,SOCK_STREAM,0)) == -1) {
        perror("socket");
        return -1;
    }

    host.sin_family = AF_INET;
    host.sin_addr.s_addr = htonl(INADDR_ANY); //WIF
    host.sin_port = htons(port);

    option = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option, sizeof(option));

    if(bind(listen_sock,(struct sockaddr*) &host,sizeof(host)) == -1) {
        perror("bind");
        return -1;
    }

    if(listen(listen_sock,MAX_BACKLOG) == -1) {
        perror("listen");
        return -1;
    }

    return listen_sock;
}

int connect_peer(resource_id *uri,int *sock) {
    struct addrinfo *info = NULL; //WIF does this need freeing???
    struct addrinfo hint;

    memset(&hint,0,sizeof(hint));
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;

    sock = socket(AF_INET,SOCK_STREAM,0);
    getaddrinfo(uri->host,uri->port,&hint,&info);

    if(connect(sock,(struct sockaddr*) info->ai_addr,info->ai_addrlen) == -1) {
        return -1;        
    }

    return 0;
}