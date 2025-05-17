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
    if(!strcmp(command,"status")||!strcmp(command,"shutdown")) {
        return false;
    }

    if(strstr(response,"Directory not monitored") != NULL) return false;
    if(strstr(response,"Already in queue") != NULL) return false;
    if(strstr(response,"Sync already in progress") != NULL) return false;
    return true;
}

int main(int argc,char **argv) {
    int opt,flag,fifos[2];
    char line[MAX_LINE],buffer[BUFSIZ];
    FILE *log;
    char *log_fn = NULL;
    ssize_t size;
    flag = 0;

    signal(SIGPIPE,SIG_IGN);
    // Parse arguments
    opt = getopt(argc,argv,"l:");
    if(opt == 'l') log_fn = optarg;

    if (argc != 3 || !log_fn) {
        fprintf(stderr,"Usage:\n ./fss_console -l <console_logfile>\n");
        return -1;
    }

    // Opening pipe to manager
    log = fopen(log_fn,"w");
    fifos[0] = open(FSS_IN,O_WRONLY | O_NONBLOCK);
    if(fifos[0] <= 0) {
        printf("Fss Manager not listening.\nExiting...\n");
        exit(1);
    }

    while(1) {
        printf("\n%s","> ");
        fgets(line,MAX_LINE,stdin);

        // Remove new line
        line[strcspn(line, "\n")] = 0;

        // Log command
        fprintf(log,"Command %s\n",line);
        fflush(log);
        // Send command to manage
        if (send_msg(line,strlen(line) + 1,fifos[0]) == -1) {
            printf("Error sending command.\nExiting...\n");
            exit(1);
        }
        
        // Open pipe to read response from manager (if connection hasn't been established already)
        if (!flag) {
            fifos[1] = open(FSS_OUT,O_RDONLY);
            flag = 1;
        }

        // Read and log responses until a whole message group has been received
        while((size = receive_msg(fifos[1],buffer)) > 0) {
            if(!strcmp(buffer,MSG_END)) break;
            if(loggable(line,buffer)) {
                fprintf(log,"%s",buffer);
                fflush(log);
            }
            printf("%s",buffer);

        }
        // Close console if shutdown has been issued
        if(!strcmp(line,"shutdown")) {
            break;
        }
    }
    fclose(log);
    return 0;

}