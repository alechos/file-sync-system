#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "utils.h"
#include "client.h"
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>

/* Converts command passed from console to COMMAND_TYPE.
Returns INVALID_COM on failure.*/
CLIENT_OP get_client_op(char *op) {
    if(!strcmp("LIST",op)) return LIST;
    if(!strcmp("PUSH",op)) return PUSH;
    if(!strcmp("PULL",op)) return PULL;

    return INVLD_OP;
}

/* Converts command to string and stores in line.
Returns -1 if command is INVALID_COM.*/
int parse_header(char *header,header_info *head) {
    
    char temp[PACKET_SIZE];
    char *arg,*buff_ptr;
    buff_ptr = temp;

    header = strdup(header);
    arg = strtok_r(header," ",&buff_ptr);
    head->op = get_client_op(arg);

    if(head->op == INVLD_OP) {
        free(header);
        return -1;
    }
    
    arg = strtok_r(NULL," ",&buff_ptr);
    snprintf(head->path,MAX_PATH_SIZE,"%s",arg);
    
    if (head->op == PUSH) {
        arg = strtok_r(NULL," ",&buff_ptr);
        head->chunk_size = atol(arg);
    }

    free(header);
    return 0;

}

int get_error(int err,char *buff) {
    
    if (strerror_r(err,buff,PACKET_SIZE) == 0) return 0;
    
    snprintf(buff,PACKET_SIZE,"%s","Unknown Error.");
    return -1;
}

int send_file(int src,int dst) {
    char buffer[PACKET_SIZE];
    char err_buff[PACKET_SIZE];

    ssize_t total_r = 0;

    // Attempt to read PACKET_SIZEd chunks into buffer until EOF
    while((total_r = read(src,buffer,PACKET_SIZE)) > 0) {
        // write the chunk read in dst until total read bytes have been written
        if( send_msg(buffer,total_r,dst) == -1) return -1; //return -1 if peer unreachable
    
    }

     if(total_r == -1) { 
        get_error(errno,err_buff);
        send_msg(MSG_ERR,strlen(MSG_ERR),dst);
        send_msg(err_buff,strlen(err_buff) + 1,dst);
        return -1;
    }
    return 0;
}

int pull(char *path, int sock) {
    int src;
    struct stat st;
    char buff[PACKET_SIZE];
    size_t offset;

    if (stat(path,&st) != 0) {

        offset = snprintf(buff,PACKET_SIZE,"-1");    
        send_msg(buff,offset+1,sock);

        get_error(errno,buff);
        send_msg(buff,strlen(buff)+1,sock);
        return -1;
    }

    src = open(path,O_RDONLY);

    if(src == -1) {
        offset = snprintf(buff,PACKET_SIZE,"-1");    
        send_msg(buff,offset+1,sock);

        get_error(errno,buff);
        send_msg(buff,strlen(buff)+1,sock);
        return -1;
    }

    offset = snprintf(buff,PACKET_SIZE,"%ld",st.st_size);
    buff[offset++] = ' ';

    send_msg(buff,offset + 1,sock); //send file size
    if (send_file(src,sock) == -1) {
        close(src);
        return -1;
    }

    close(src);
    return 0;
}

int list(char *path,int sock) {
    DIR *dir;
    struct dirent *entry;
    char files[PACKET_SIZE] = {0};
    int offset = 0;

    dir = opendir(path);
    if(!dir) {
        files[offset] = '.';
        send_msg(files,strlen(files) + 1,sock); 
        return -1;       
    }

    while((entry = readdir(dir)) != NULL && offset < PACKET_SIZE) {
        if(entry->d_name[0] == '.') continue;
        offset += snprintf(files + offset, PACKET_SIZE - offset, "%s\n", entry->d_name);
    }
    files[offset] = '.';

    if(send_msg(files,strlen(files) + 1,sock) == -1) {
        return -1;
    }

    closedir(dir);
    return 0;
}

int receive_file(int file,int sock,size_t initial_size) {
    char packet[PACKET_SIZE];
    header_info header;
    header.chunk_size = initial_size;
    
    while(header.chunk_size != 0) {
        receive_msg(sock,packet);
        if(!strcmp(packet,MSG_END)) {
            return -1;
        }
        write_buff(packet,header.chunk_size,file);

        // parse next header
        receive_msg(sock,packet);
        parse_header(packet,&header);
    }

    return 0;

}

int push(header_info *header,int sock) {
    int fd;
    char msg[PACKET_SIZE], err_buff[PACKET_SIZE];

    if(header->chunk_size == -1) { //when would this happen?? WIF
        fd = open(header->path,O_WRONLY | O_CREAT | O_TRUNC,0644);
        receive_msg(sock,msg);  //read actual header
        parse_header(msg,header);

    } else {
        fd = open(header->path,O_WRONLY | O_CREAT | O_APPEND,0644);
    }

    if(fd == -1) {
        get_error(errno,err_buff);
        send_msg(MSG_ERR,strlen(MSG_ERR),sock);
        send_msg(err_buff,strlen(err_buff) + 1,sock);
        close(sock);
        return -1;
    }    
    receive_file(fd,sock,header->chunk_size);
    close(fd);
    return 0;
}

/* Handles communication with connected peer*/
int handle_coms(int com_sock) {
    ssize_t msg_size;
    header_info header;
    char packet[PACKET_SIZE];
    int flag = 1;
    //WIF i dont think the loop is needed, we only handle one op per connection anyways
    
    while(flag) {
        //receive command
        flag = ((msg_size = receive_msg(com_sock,packet)) > 0);
        if(!flag) return -1; //peer disconnected, exit
        //parse header
        parse_header(packet,&header);

        switch (header.op)
        {
        case LIST:
            list(header.path,com_sock);
            break;
        case PULL:
            pull(header.path,com_sock);
            break;
        case PUSH:
            push(&header,com_sock);
            break;
        default:
            send_msg("Wrong command\n",15,com_sock);
            send_msg(MSG_END,strlen(MSG_END)+1,com_sock);
            break;
        }

    }

    return 0;
}

/* Thread blocks accepting new connections from given listening socket.
    When a new connection is accepted, handle operation.*/
void* handle_peer(void *arg) {

    struct sockaddr_in peer;
    socklen_t addrlen;
    int com_sock;
    int listen_sock = *((int*) arg);
    addrlen = sizeof(peer);

    while(1) {

        com_sock = accept(listen_sock,(struct sockaddr*) &peer,&addrlen);
        if(com_sock < 0) {
            if (errno == EBADF || errno == EINVAL) {
                break;
            }
            continue;
        }
        handle_coms(com_sock);
        close(com_sock);
    }
    return NULL;
}

int main(int argc, char** argv) {
    pthread_t workers[MAX_WORKERS];
    int port,opt,listen_sock;
    port = -1;

    opt = getopt(argc,argv,"p:");
    if(opt == 'p') port = atoi(optarg);

    if (argc != 3 || port == -1) {
        fprintf(stderr,"Usage:\n ./client -p <port_number>\n");
        return -1;
    }
    listen_sock = get_listener(port);

    for(int i = 0; i < MAX_WORKERS;i++) {
        pthread_create(&workers[i],NULL,handle_peer,(void*) &listen_sock); //listen sock is local! WIF
    }

    printf("Running, enter any character to shut down.\n");
    getchar();
    
    shutdown(listen_sock, SHUT_RDWR);
    printf("Shutting down\n");

    for(int i = 0; i < MAX_WORKERS;i++) {
        pthread_join(workers[i],NULL);
    }
    return 0;
}