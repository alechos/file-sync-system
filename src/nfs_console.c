#define _POSIX_C_SOURCE 200809L
#include "command.h"
#include "config.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>


/* Detrmines whether the output of a command should be logged in the console log files*/
bool loggable(char* line,char* response) {
    char *command;
    command = strtok(line," ");
    if(!strcmp(command,"shutdown")) {
        return false;
    }

    if(strstr(response,"Directory not being synchronized") != NULL) return false;
    if(strstr(response,"Already in queue") != NULL) return false;
    return true;
}

int parse_args(char **log,char **host, char **port, int argc,char **argv) {
    int opt;
    while((opt = getopt(argc,argv,"l:h:p:"))!= -1) {
        switch (opt) {
        case 'l':
            *log = optarg;
            break;
        case 'h':
            *host = optarg;
            break;
        case 'p':
            *port = optarg;
            break;
        default:
            return -1;
        }
    }

    if(!(*log) || !(*host) || !(*port)) {
        fprintf(stderr,
            "Usage:\n"
            "  ./nfs_console -l <console-logfile>\n"
            "                -h <host_IP>\n"
            "                -p <host_port\n"
        );
        return -1;
    } 

    return 0;
}
int main(int argc,char **argv) {
    int sock;
    char *host_ip, *host_port;
    char packet[PACKET_SIZE],buffer[BUFSIZ];
    resource_id uri;
    FILE *log;
    char *log_fn = NULL;
    ssize_t size;

    // Parse arguments
    if (parse_args(&log_fn,&host_ip,&host_port,argc,argv) == -1) return -1;

    snprintf(uri.host,MAX_HOST_SIZE,"%s",host_ip);
    snprintf(uri.port,MAX_PORT_SIZE,"%s",host_port);
    
    // Opening pipe to manager
    if(connect_peer(&uri,&sock) == -1) {
        printf("Error connecting to host.\n");
        return -1;
    }

    log = fopen(log_fn,"w");
    

    while(1) {
        printf("\n%s","> ");
        fgets(packet,PACKET_SIZE,stdin);

        // Remove new line
        packet[strcspn(packet, "\n")] = 0;

        // Log command
        fprintf(log,"Command %s\n",packet);
        fflush(log);

        // Send command to manager
        if (send_msg(packet,strlen(packet) + 1,sock) == -1) {
            printf("Error sending command.\nExiting...\n");
            exit(1);
        }
        
        // Read and log responses until a whole message group has been received
        while((size = receive_msg(sock,buffer)) > 0) {
            if(!strcmp(buffer,MSG_END)) break;
            if(loggable(packet,buffer)) {
                fprintf(log,"%s",buffer);
                fflush(log);
            }
            printf("%s",buffer);

        }
        // Close console if shutdown has been issued
        if(!strcmp(packet,"shutdown")) {
            break;
        }
    }
    fclose(log);
    return 0;

}