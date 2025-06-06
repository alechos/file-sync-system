#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include "command.h"
#include "fss_manager_stings.h"
#include "worker.h"
#include "worker_list.h"
#include "config.h"
#include "job.h"
#include "queue.h"
#include "sync.h"
#include "sync_mem.h"
#include "exec_report.h"
#include "utils.h"

#include <string.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <poll.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>

#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

FILE *LOG = NULL;

int generate_msg(char*,char*,char*,char*,int,ssize_t);
int log_and_print(char*,size_t);
int broadcast(int,char*,size_t);

/*  Parses program aguments.
    Returns -1 on failure.
*/
int parse_args(char **logfile,char **cfgfile,int *max_n,int *port,size_t *buff_sz,int argc,char **argv) {
    int opt;
    while((opt = getopt(argc,argv,"l:c:n:p:b"))!= -1) {
        switch (opt) {
        case 'l':
            *logfile = optarg;
            break;
        case 'c':
            *cfgfile = optarg;
            break;
        case 'n':
            *max_n = strtol(optarg,NULL,10);
            break;
        case 'p':
            *port = optarg;
            break;
        case 'b':
            *buff_sz = optarg;
            break;
        default:
            return -1;
        }
    }

    if(!(*logfile) || !(*cfgfile)) {
        fprintf(stderr,
            "Usage:\n"
            "  ./fss_manager -l <manager_logfile>\n"
            "                -c <config_file>\n"
            "                -n <worker_limit>\n"
            "                -p <port>\n"
            "                -b <buff_size>\n"

        );
        return -1;
    } 

    return 0;
}


/*
    Concatenate given directory and filename into full_path.
    Returns -1 on failure.
 */
int get_file_path(char *dir,char *filename,char *full_path) {
    int result;
    result = snprintf(full_path,MAX_PATH_SIZE,"%s/%s",dir,filename);
    if(result < 0 || result >= MAX_PATH_SIZE) {
        return -1;
    }
    return 0;
}



/* Extracts details for a completed job into ```char *details``` based on an exec Report
   and a (potentially empty) error buffer.*/
int extract_details(Job job,Report report,char *err_msg,char *details) {
    if(!strcmp(job.fn,"ALL")) {
        if(report.skipped == 0) {
            //All files copied
            snprintf(details,MAX_MSG_SIZE,"%d files copied",report.copied);
        } else {
            //File was skipped
            snprintf(details,MAX_MSG_SIZE,"%d files copied, %d skipped",report.copied,report.skipped);
        }
        return 0;
    }

    if(report.err_len == 0) {
        //No errors
        snprintf(details,MAX_MSG_SIZE,"File: %s",job.fn);
    } else {
        //Errors have been recorded
        snprintf(details,MAX_MSG_SIZE,"File: %s - %s",job.fn,err_msg); 
    }

    return 0;
}

/* Writes a log entry for a completed job in the loaded logfile LOG.*/
int log_job(Job job,Report report,char *err_msg,pid_t pid) {
    char details[MAX_MSG_SIZE],tmstmp[32],op_name[9],result[32];

    switch (report.status) {
    case SUCCESS:
        strcpy(result,"SUCCESS");
        break;
    case PARTIAL:
        strcpy(result,"PARTIAL");
        break;
    case ERROR:
        strcpy(result,"ERROR");
        break;
    }
    get_op_name(job.op,op_name);
    get_timestamp(tmstmp,32,-1);
    extract_details(job,report,err_msg,details);
    //Log entry
    fprintf(LOG,LOG_ENTRY_STR,tmstmp,job.sd,job.td,pid,op_name,result,details);
    fflush(LOG);
    return 0;
}


/* Broadcast message ```msg``` of length ```len``` to stdout,the log and to the console
    connected through ```out```.*/
int broadcast(int out,char* msg,size_t len) {   
    fprintf(LOG,"%s",msg);
    printf("%s",msg);
    if(send_msg(msg,len + 1,out)) {
        printf("Could not broadcast\n");
        return -1; 
    }
    return 0;
}

/* Only log and print message.*/
int log_and_print(char *msg,size_t len) {
    if(fprintf(LOG,"%s",msg) < 0) return -1;
    printf("%s",msg);
    return 0;
}

/*Onlt print and send message to console.*/
int print_and_send(int out,char *msg,size_t len) {
    printf("%s",msg);
    if(send_msg(msg,len + 1,out)) {//plus one for null
        printf("Cound not send\n");
        return -1; //maybe fix here too (len)
    }
    return 0;
}

/*Generates a log,console,stdout message depending on parameters passed
    Writes message into ```buff``` bsed on format string ```frmt_str```.*/
int generate_msg(char *buff,char *frmt_str,char *src,char *dst,int errs,ssize_t size) {
    char timestmp[32] = {0};
    get_timestamp(timestmp,size,-1);

    if(!src) { //
        snprintf(buff,size,frmt_str,timestmp);
        return 0;
    } else if(!dst) {
        snprintf(buff,size,frmt_str,timestmp,src);
      
    } else if(size == -1) {
        snprintf(buff,MAX_MSG_SIZE,frmt_str,timestmp,src,dst);
    } else {
        snprintf(buff,size,frmt_str,timestmp,src,dst,errs);
    }

    return 0;
}
    
/* Converts command passed from console to COMMAND_TYPE.
Returns INVALID_COM on failure.*/
COMMAND_TYPE arg_to_comm(char *arg) {
    if(!strcmp("add",arg)) return ADD;
    if(!strcmp("cancel",arg)) return CANCEL;
    if(!strcmp("shutdown",arg)) return SHUTDOWN;
    return INVALID_COM;
}

/* Converts string to command and stores in command.
Returns -1 if command is INVALID_COM.*/
int parse_command(char *line,Command *command) {
    char *arg;
    arg = strtok(line," ");
    command->com = arg_to_comm(arg);
    if(command->com == INVALID_COM) {
        return -1;
    }

    if(command->com != SHUTDOWN) {
        arg = strtok(NULL," ");
        snprintf(command->source,MAX_URI_LEN,"%s",arg);
    }

    if(command->com == ADD) {
        arg = strtok(NULL," ");
        snprintf(command->destination,MAX_URI_LEN,"%s",arg);
    }

    return 0;

}

/*Handler for add command by console. Begins monitoring for a new or 
stopped direcotry and executes a full sync job if not already in queue.*/
int console_add(int out,Command com,JobQueue jobs, SyncMem sm_info) {
    SyncEntry entry;
    Job sync_job;
    time_t time_stamp;
    int wd,entry_exists;
    char *src = com.source;
    char *dst = com.destination;
    char msg_buff[MAX_MSG_SIZE] = {0};

    entry_exists = (sm_get_entry(sm_info,&entry,com.source) != -1);
    time_stamp = (entry_exists) ? entry.sync_timestamp : time(NULL); 
    
    if (entry_exists && strcmp(entry.dst.,dst) ) { // New destination doesn't match previous
        // End message and refuse command...
        send_msg(MSG_END,strlen(MSG_END) + 1,out);
        return -1;
    }
    // If entry is not in sm_info or it's inactive start monitoring
    if(!entry_exists || entry.status != ACTIVE) {
        if((wd = inotify_add_watch(event_fd,src,WATCH_OPTS)) != -1 ) {
            entry = create_sync_entry(src,dst,time_stamp,ACTIVE,wd,0);
            sm_add_entry(sm_info,entry);
            
            //if entry doesn't exist already notify all channels
            if(!entry_exists) {
                generate_msg(msg_buff,ADDED_DIR_STR,src,dst,-1,MAX_MSG_SIZE);
                broadcast(out,msg_buff,strlen(msg_buff) + 1);
            }

            generate_msg(msg_buff,MON_STARTED_STR,src,dst,-1,MAX_MSG_SIZE);
            broadcast(out,msg_buff,strlen(msg_buff) + 1);
        }
    }
    
    // if not actively syncing source directory, execute a full sync
    if(!entry_exists || !wl_dir_stat(wl,src) || jq_in_queue(jobs,src)) {
        sync_job = create_job(src,dst,"ALL",FULL);
        issue_job(sync_job,wl,jobs,sm_info);
    } else {
        //notify in case of sync already in queue or being executed
        generate_msg(msg_buff,IN_QUEUE_STR,src,NULL,-1,MAX_MSG_SIZE);
        print_and_send(out,msg_buff,strlen(msg_buff)+1);
    }

    send_msg(MSG_END,strlen(MSG_END) + 1,out);

    return 0;
}

/* Handler for cancel command from the console.
Returns -1 on error. Cancels monitoring for an existing directory.*/
int console_cancel(int out,Command com, SyncMem sm_info,int fd) {
    SyncEntry entry;
    char *src = com.source;
    char buff[MAX_MSG_SIZE] = {0};

    if(sm_get_entry(sm_info,&entry,src) == -1 || entry.status != ACTIVE) {
        generate_msg(buff,NOT_MON_STR,src,NULL,-1,MAX_MSG_SIZE);
        print_and_send(out,buff,strlen(buff) + 1);
        send_msg(MSG_END,strlen(MSG_END) + 1,out);

        return -1;
    }
    // WIF ???
    //remove inotify watch for src directory in command
    //if(inotify_rm_watch(fd,entry.wd) != -1) {
    //    entry.status = STOPPED;
    //   sm_add_entry(sm_info,entry);
    //    generate_msg(buff,CANCEL_MON_STR,src,NULL,-1,MAX_MSG_SIZE);
    //    broadcast(out,buff,strlen(buff)+1);
    //}
    send_msg(MSG_END,strlen(MSG_END) + 1,out);
    return 0;
}


/* Handler for shutdown command from the console. Inititates shutdown and waits
for all jobs to finish before exiting smoothly.*/
int console_shutdown(int out,SyncMem sm,WorkerList wl,JobQueue jobs) {
    char buff[MAX_MSG_SIZE] = {0};
    int sig;
    sigset_t mask;

    generate_msg(buff,MAN_SHUTDOWN_STR,NULL,NULL,-1,MAX_MSG_SIZE);
    print_and_send(out,buff,strlen(buff) + 1);

    generate_msg(buff,WORKER_WAIT_STR,NULL,NULL,-1,MAX_MSG_SIZE);
    print_and_send(out,buff,strlen(buff) + 1);

    generate_msg(buff,QUEUE_WAIT_STR,NULL,NULL,-1,MAX_MSG_SIZE);
    print_and_send(out,buff,strlen(buff) + 1);

    // prepare SIGCHLD for sigwait
    sigemptyset(&mask);
    sigaddset(&mask,SIGCHLD);
    sigprocmask(SIG_BLOCK,&mask,NULL);

    // handle remaining jobs by emptying the worker list and the job queue
    while((wl->size + jq_size(jobs)) > 0) {
        sigwait(&mask,&sig);    // When SIGCHLD is received reap childs that exited
        reap_workers(wl,jobs,sm,out);
    }

    generate_msg(buff,SHUTDOWN_COMPLETE_STR,NULL,NULL,-1,MAX_MSG_SIZE);
    print_and_send(out,buff,strlen(buff) + 1);

    send_msg(MSG_END,strlen(MSG_END)+1,out);

    return 0;
}

/* Proccesses commands passed from the console. 
Returns -1 on failure, 0 on success and 1 when a shutdown has been issued.*/
int process_command(int sock,SyncMem sm,JobQueue jobs) {
    char buff[PACKET_SIZE];
    Command command;
    ssize_t size;
    // Receive command from console
    if ((size = receive_msg(sock,buff)) > 0) {
        buff[size] = 0;
        parse_command(buff,&command);
    }

    switch (command.com) {
    case ADD:
        return console_add(sock,command,jobs,sm);
    case CANCEL:
        return console_cancel(sock,command,sm);
    case SHUTDOWN:
        // If shutdown succesfully return 1, signify shutdown
        if(!console_shutdown(sock,sm,jobs)) return 1;
    default:
        send_msg("Invalid Command\n",17 + 1,sock);
        send_msg(MSG_END,strlen(MSG_END) + 1,sock);
        break;
    }

    return -1;
}

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

void* execute_job(void* arg) {
    int src,dst;
    Job job;
    JobQueue jq = (JobQueue) arg;
    while(1) {
        jq_dequeue(jq,&job); // blocking till job available, thread safe

        connect_peer(&job.src,&src);
        connect_peer(&job.dst,&dst);
        
        pull_push(src,dst,&job.fn);
    }
}

//free return array
int spawn_workers(pthread_t* pool,int n,JobQueue jq) {

    for(int i = 0; i < n; i++) {
        pthread_create(&pool[i],NULL,execute_job,(void*) jq); // WIF pthread_create fails
    }

    return 0;
}

// buff must be at least of size PACKET_SIZE
int get_list(char *buff,char *path,int sock) {
    char msg[PACKET_SIZE];
    snprintf(msg,PACKET_SIZE - 1,LIST_OP_STR,path);
    if(send_msg(msg,strlen(msg) + 1,sock) != - 1) {
        if(receive_msg(sock,buff) <= 0) {
            return -1;
        }
        return 0;
    }

    return -1;
}

ssize_t get_buff_line(char* line, char** buff) {
    char *ptr;
    size_t offset;

    ptr = strchr(*buff,'\n');
    if (ptr == NULL) {
        return -1;
    }

    *ptr = '\0';
    offset = ptr - *buff;
    memcpy(line,*buff,offset);
    *buff = ++ptr;

    return offset;
}

int issue_jobs(resource_id *src,resource_id *dst,JobQueue jobs) {
    Job job;
    int sock;
    char file[MAX_FILENAME_SIZE];
    char list[PACKET_SIZE];

    if (connect_peer(src,sock) == -1) return -1;
    if (get_list(list,src->dir,sock) == -1) return -1;

    while(get_buff_line(file,&list) != -1) {
        job = create_job(src,dst,file);    
        jq_enqueue(jobs,job);
    }

    return 0;
}

/* Initializes fss manager data_strcutures based on config file given.
    Returns -1 on failure.
*/
int init_manager(const char *conf_file_path,const char *log,SyncMem sm_info,JobQueue jobs) {
    FILE* config_file;
    SyncEntry entry;
    Job sync_job;
    int wd;
    char buff[MAX_URI_LEN*2 + 1],msg_buff[MAX_MSG_SIZE];
    char src[MAX_URI_LEN],dst[MAX_URI_LEN];

    config_file = fopen(conf_file_path,"r");   
    LOG = fopen(log,"w");

    if (!(config_file) || !(LOG)) {
        perror("init_manager: ");
        return -1;
    }

    // Iterate through configuration file lines
    while(fgets(buff,MAX_URI_LEN*2 + 1,config_file)) {
        buff[strcspn(buff, "\n")] = 0;
        // Extract source and destination paths
        snprintf(src,MAX_URI_LEN,"%s",strtok(buff," "));
        snprintf(dst,MAX_URI_LEN,"%s",strtok(NULL," "));
        
        entry = create_sync_entry(src,dst,time(NULL),ACTIVE);    
        issue_jobs(&entry.src,&entry.dst,jobs);

        // Log entries
        //generate_msg(msg_buff,ADDED_DIR_STR,src,dst,-1,MAX_MSG_SIZE);
        //log_and_print(msg_buff,strlen(msg_buff) + 1);
           
        //generate_msg(msg_buff,MON_STARTED_STR,src,dst,-1,MAX_MSG_SIZE);
        //log_and_print(msg_buff,strlen(msg_buff) + 1);

        // Catalogue the monitoring in sm_info and enque corresponding sync job
        sm_add_entry(sm_info,entry);
    }
    fclose(config_file);
    return 0;
}

int main(int argc, char **argv) {
    struct sockaddr_in manager_addr; //maybe i have to change this WIF
    socklen_t addrlen;

    pthread_t *worker_pool;
    SyncMem watch_dirs;
    JobQueue jobs;
    Job new_job;
    Command command;

    size_t buff_sz;
    int port,listener_sock,console_sock;
    int max_n,flag;
    char cmd_buff[PACKET_SIZE];

    char *logfile = NULL,*cfgfile =NULL;
    max_n = 5;
    buff_sz = 10;

    if(parse_args(&logfile,&cfgfile,&max_n,&port,&buff_sz,argc,argv) < 0) {
        return -1;
    }

    // Create sync_info struct and Job queue stucts
    watch_dirs = sm_create();
    jobs = jq_create(buff_sz);
    worker_pool = malloc(max_n*sizeof(pthread_t));

    // Initialize system by loading config entries and preparing jobs
    init_manager(cfgfile,logfile,watch_dirs,jobs);
    worker_pool = spawn_workers(worker_pool,max_n,jobs); // WIF maybe do it after init_manager

    listener_sock = get_listener(port);
    addrlen = sizeof(manager_addr);

    while(1) {
        console_sock = accept(listener_sock,(struct sockaddr*) &manager_addr,&addrlen);
        if(console_sock < 0) {
            if (errno == EBADF || errno == EINVAL) {
                printf("Failure establishing accepting socket\n");
                break;
            } else {
                continue;
            }
        }

        flag = 0;
        while(flag == 0) {
            flag = process_command(console_sock,watch_dirs,jobs)

        }

    }

    //Free resources
    sm_del(watch_dirs);
    jq_del(jobs);
    fclose(LOG);

    return 0;
}
