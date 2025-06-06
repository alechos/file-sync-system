#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "../include/config.h"
#include "../include/utils.h"
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>

#define PUSH_OP_STR "PUSH %s %ld "
#define PULL_OP_STR "PULL %s "


int transfer_file(int src,int dst,char *path,ssize_t file_size) {
    char buffer[PACKET_SIZE];
    char header[PACKET_SIZE];

    ssize_t total_r = 0;

    while(file_size > 0 && (total_r = receive_msg(src,buffer)) > 0 ){
        if(!strcmp(buffer,MSG_END)) {
            send_msg(MSG_END,strlen(MSG_END)+1,dst);
            close(src);
            close(dst);
            return -1;
        }
        snprintf(header,PACKET_SIZE,PUSH_OP_STR,path,total_r);
        send_msg(header,strlen(header) + 1,dst);
        send_msg(buffer,total_r,dst);
        file_size-=total_r;

    }
    // WIF: what about other errors here??
    if(file_size <= 0) {
        snprintf(header,PACKET_SIZE,PUSH_OP_STR,path,0);
        send_msg(header,strlen(header) + 1,dst);
        return 0;
    }
    //send_msg(HEADER)
    //close(src);
    //close(dst);
    //WIF: client isnt notified?
    return -1;
}

int pull_push(int src,int dst,char *path) {
    char buff[PACKET_SIZE];
    ssize_t file_size;
    char *end_ptr;

    //issue a pull
    snprintf(buff,PACKET_SIZE,PULL_OP_STR,path);
    send_msg(buff,strlen(buff) +1,src);

    receive_msg(src,buff);
    file_size = strtol(buff,&end_ptr,10);

    if(file_size == -1) {
        receive_msg(src,buff);
        printf("Error: %s\n",buff);
        return -1;
    }
    
    return transfer_file(src,dst,path,file_size);

}

int main(int argc, char **argv) {
    int sock1,sock2;
    struct addrinfo* info = NULL; //WIF does this need freeing???
    struct addrinfo hint;
    char packet[PACKET_SIZE];

    memset(&hint,0,sizeof(hint));
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;

    sock1 = socket(AF_INET,SOCK_STREAM,0);
    getaddrinfo("127.0.0.1","7777",&hint,&info);

    if(connect(sock1,(struct sockaddr*) info->ai_addr,info->ai_addrlen) == -1) {
        perror("connect");
    }

    sock2 = socket(AF_INET,SOCK_STREAM,0);
    getaddrinfo("127.0.0.1","6666",&hint,&info);

    if(connect(sock2,(struct sockaddr*) info->ai_addr,info->ai_addrlen) == -1) {
        perror("connect");
    }

    memset(packet,0,PACKET_SIZE);
    pull_push(sock1,sock2,"test/papapipi.txt");

    close(sock1);
    close(sock2);


}