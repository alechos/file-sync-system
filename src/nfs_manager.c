#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include "command.h"
#include "nfs_manager_stings.h"
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

int generate_msg(char*,char*,char*,char*,ssize_t);
int log_and_print(char*);
int broadcast(int,char*,size_t);

/*  Parses program aguments.
    Returns -1 on failure. */
int parse_args(char **logfile,char **cfgfile,int *max_n,int *port,size_t *buff_sz,int argc,char **argv) {
    int opt;

    while((opt = getopt(argc,argv,"l:c:n:p:b:"))!= -1) {
        switch (opt) {
        case 'l': //parse logfile name
            *logfile = optarg;
            break;
        case 'c': //parse config file name
            *cfgfile = optarg;
            break;
        case 'n':   //parse max number of woekrs
            *max_n = strtol(optarg,NULL,10);
            break;
        case 'p': //parse port
            *port = strtol(optarg,NULL,10);
            break;
        case 'b': //parse job queue slot count
            *buff_sz = strtol(optarg,NULL,10);;
            break;
        default:  //invalid format
            return -1;
        }
    }

    if(!(*logfile) || !(*cfgfile)) {
        fprintf(stderr,
            "Usage:\n"
            "  ./nfs_manager -l <manager_logfile>\n"
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
int extract_details(Job job,Report report,char *err_msg,char *pulled,char* pushed) {


    //Extract pull op details

    if(report.pulled <= 0) { //pull error occured
        snprintf(pulled,MAX_MSG_SIZE,"File: %s - %s",job.fn,err_msg); 
    } else {
        snprintf(pulled,MAX_MSG_SIZE,"%zd bytes pulled",report.pulled);
    }

    //Extract push op details
    if(report.pushed <= 0) { //push error occured
        snprintf(pushed,MAX_MSG_SIZE,"File: %s - %s",job.fn,err_msg); 
    } else {
        snprintf(pushed,MAX_MSG_SIZE,"%zd bytes pushed",report.pushed);
    }

    return 0;
}

/* Writes a log entry for a completed job in the loaded logfile LOG.*/
int log_job(Job job,Report report,char *err_msg,pthread_t thread_id) {
    char tmstmp[32],result[32];
    char pulled[MAX_MSG_SIZE],pushed[MAX_MSG_SIZE];
    char src_uri[MAX_URI_LEN], dst_uri[MAX_URI_LEN];    

    switch (report.status) {
    case SUCCESS:
        strcpy(result,"SUCCESS");
        break;
    case ERROR:
        strcpy(result,"ERROR");
        break;
    default:
        return -1;
    }

    get_timestamp(tmstmp,32,-1);

    extract_details(job,report,err_msg,pulled,pushed);

    uri_string(&job.src,job.fn,src_uri);
    uri_string(&job.dst,job.fn,dst_uri);

    //Log entry
    fprintf(LOG,LOG_ENTRY_STR,tmstmp,src_uri,dst_uri,(unsigned long) thread_id,"PULL",result,pulled);

    if(report.pushed >0) {
        fprintf(LOG,LOG_ENTRY_STR,tmstmp,src_uri,dst_uri,(unsigned long) thread_id,"PUSH",result,pushed);
    }

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
int log_and_print(char *msg) {
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
    Writes message into ```buff``` bsed on format string ```frmt_str```.
    Returns -1 on error. */
int generate_msg(char *buff,char *frmt_str,char *src,char *dst,ssize_t size) {
    char timestmp[32] = {0};
    get_timestamp(timestmp,size,-1);

    if(!src) { //default timestamped string
        snprintf(buff,size,frmt_str,timestmp);
    } else if(!dst) { //timestamped source string
        snprintf(buff,size,frmt_str,timestmp,src);
    } else { //full timestamped source-dist string
        snprintf(buff,size,frmt_str,timestmp,src,dst);
        return -1;
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

    if(!arg) {
        return -1;
    }
    command->com = arg_to_comm(arg);
    
    // Return in case of invalid command
    if(command->com == INVALID_COM) {
        return -1;
    }

    // In case command isn't shutdown a source field exists
    if(command->com != SHUTDOWN) {
        arg = strtok(NULL," ");
        snprintf(command->source,MAX_URI_LEN,"%s",arg);
    }

    // If the command is an ADD command a destination is also present
    if(command->com == ADD) {
        arg = strtok(NULL," ");
        snprintf(command->destination,MAX_URI_LEN,"%s",arg);
    }

    return 0;

}

int get_list(char *buff, char *path, int sock) {
    char msg[PACKET_SIZE];
    char filename[MAX_MSG_SIZE];
    int offset = 0;

    snprintf(msg, PACKET_SIZE - 1, LIST_OP_STR, path);
    if (send_msg(msg, strlen(msg) + 1, sock) == -1)
        return -1;

    while (1) {
        if (receive_msg(sock, filename) <= 0)
            return -1;
        if (filename[0] == '.')
            break;
        offset += snprintf(buff + offset, PACKET_SIZE - offset, "%s", filename);
    }
    return 0;
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
    line[offset] = '\0';

    *buff = ++ptr;

    return offset;
}

/*  Handler for add command by console. Begins syncing of a new direcotry 
    by filling the jobs queue with the directory's contents. 
    Returns -1 on failure.*/

int console_add(int out,Command com,JobQueue jobs, SyncMem sm_info) {
    SyncEntry entry;
    Job sync_job;
    resource_id src_uri,dst_uri;
    int peer_sock,entry_exists;
    char list_buff[PACKET_SIZE], file_path[MAX_FILENAME_SIZE];
    char full_src_path[MAX_PATH_SIZE],full_dist_path[MAX_PATH_SIZE];
    char msg_buff[MAX_MSG_SIZE] = {0};
    char *buff_ptr;

    if(parse_uri(com.source,&src_uri) == -1) return -1;
    if(parse_uri(com.destination,&dst_uri) == -1) return -1;

    entry_exists = (sm_get_entry(sm_info,&entry,&src_uri) != -1);
    
    if (entry_exists && !compare_uris(&entry.dst,&dst_uri) ) { // New destination doesn't match previous
        // End message and refuse command...
        send_msg(MSG_END,strlen(MSG_END) + 1,out);
        return -1;
    }

    //connect to source peer
    if(connect_peer(&src_uri,&peer_sock) == -1) return -1;

    // add sync entry
    entry = create_sync_entry(com.source,com.destination,time(NULL),ACTIVE);
    sm_add_entry(sm_info,entry);

    //get dir list from courcce peer
    get_list(list_buff,src_uri.dir,peer_sock);

    buff_ptr = list_buff;
    //read filenames from list line by line. get_buff_line consumes the buffer!
    while(get_buff_line(file_path,&buff_ptr) != -1) {

        //create a sync_job for read file name from source
        sync_job = create_job(&src_uri,&dst_uri,file_path);  
        snprintf(full_src_path,MAX_PATH_SIZE,"%s/%s",com.source,file_path);

        //if file already in queue notify and skip
        if(jq_in_queue(jobs,&sync_job)) {
            generate_msg(msg_buff,IN_QUEUE_STR,full_src_path,NULL,MAX_MSG_SIZE);
            print_and_send(out,msg_buff,strlen(msg_buff)+1);  
            continue;      
        }

        //add job in queue and notify
        jq_enqueue(jobs,sync_job);
        
        snprintf(full_dist_path,MAX_PATH_SIZE,"%s/%s",com.destination,file_path);
        generate_msg(msg_buff,ADDED_DIR_STR,full_src_path,full_dist_path,MAX_MSG_SIZE);
        broadcast(out,msg_buff,strlen(msg_buff) + 1);
    }

    send_msg(MSG_END,strlen(MSG_END) + 1,out);

    return 0;
}

/* Handler for cancel command from the console.
Cancels synchornization of files in queue.*/
int console_cancel(int out,Command com, SyncMem sm_info,JobQueue jobs) {
    char buff[MAX_MSG_SIZE] = {0};

    //cancel jobs matching source dir
    if(jq_cancel(jobs,com.source) == -1) {
        //notify in case dir doesnt exist
        generate_msg(buff,NOT_MON_STR,com.source,NULL,MAX_MSG_SIZE);
        print_and_send(out,buff,strlen(buff) + 1);
        send_msg(MSG_END,strlen(MSG_END) + 1,out);    
        return 0;
    }
    //notify successfull cancelation
    generate_msg(buff,CANCEL_MON_STR,com.source,NULL,MAX_MSG_SIZE);
    broadcast(out,buff,strlen(buff)+1);

    send_msg(MSG_END,strlen(MSG_END) + 1,out);
    return 0;
}


/* Handler for shutdown command from the console. Inititates shutdown and waits
for all jobs to finish before exiting smoothly.*/
int console_shutdown(int out,SyncMem sm,JobQueue jobs,pthread_t *pool,size_t thread_slots) {
    char buff[MAX_MSG_SIZE] = {0};

    generate_msg(buff,MAN_SHUTDOWN_STR,NULL,NULL,MAX_MSG_SIZE);
    print_and_send(out,buff,strlen(buff) + 1);

    generate_msg(buff,WORKER_WAIT_STR,NULL,NULL,MAX_MSG_SIZE);
    print_and_send(out,buff,strlen(buff) + 1);

    generate_msg(buff,QUEUE_WAIT_STR,NULL,NULL,MAX_MSG_SIZE);
    print_and_send(out,buff,strlen(buff) + 1);

    // wait till job_queue empties and signal shutdown...
    jq_shutdown(jobs);

    //join threads
    for(int i = 0; i < thread_slots; i++) {
        pthread_join(pool[i],NULL);
    }

    //notify shutdown completion
    generate_msg(buff,SHUTDOWN_COMPLETE_STR,NULL,NULL,MAX_MSG_SIZE);
    print_and_send(out,buff,strlen(buff) + 1);

    send_msg(MSG_END,strlen(MSG_END)+1,out);

    return 0;
}

/* Proccesses commands passed from the console. 
Returns -1 on failure, 0 on success and 1 when a shutdown has been issued.*/
int process_command(int sock,size_t slots,SyncMem sm,JobQueue jobs,pthread_t *pool) {
    char buff[PACKET_SIZE];
    Command command;
    ssize_t size;

    // Receive command from console
    if ((size = receive_msg(sock,buff)) > 0) {
        buff[size] = 0;
        parse_command(buff,&command);
    } else { 
        return -1;
    }

    switch (command.com) {
    case ADD:
        return console_add(sock,command,jobs,sm);
    case CANCEL:
        return console_cancel(sock,command,sm,jobs);
    case SHUTDOWN:
        // If shutdown succesfully return 1, signify shutdown
        if(!console_shutdown(sock,sm,jobs,pool,slots)) return 1;
        break;
    default:
        send_msg("Invalid Command\n",17 + 1,sock);
        send_msg(MSG_END,strlen(MSG_END) + 1,sock);
        return 0;
    }

    return -1;
}

int transfer_file(int src,int dst,Job *job,ssize_t file_size,Report *report) {
    char buffer[PACKET_SIZE];
    char header[PACKET_SIZE];
    char dst_path[PACKET_SIZE];

    ssize_t total_r = 0;
    report->status = ERROR;
    snprintf(dst_path,PACKET_SIZE,"%s/%s",job->dst.dir,job->fn);

    while(file_size > 0 && (total_r = receive_msg(src,buffer)) > 0 ){
        if(!strcmp(buffer,MSG_ERR)) { //error pulling!
            report->pulled = -1;
            report->status = ERROR;
            receive_msg(src,report->err_msg); //receive error message
            send_msg(MSG_END,strlen(MSG_END)+1,dst); //Terminate destination client
            return -1;
        }
        report->pulled+=total_r;

        snprintf(header,PACKET_SIZE,PUSH_OP_STR,dst_path,total_r);
        send_msg(header,strlen(header) + 1,dst);
        send_msg(buffer,total_r,dst);

        file_size-=total_r;

        report->pushed+=total_r;

    }
    //signal file transmission over
    if(file_size <= 0) {
        report->status = SUCCESS;
        snprintf(header,PACKET_SIZE,PUSH_OP_STR,dst_path,0);
        send_msg(header,strlen(header) + 1,dst);
        return 0;
    }

    return -1;
}

int pull_push(int src,int dst,Job *job,Report *report) {
    char buff[PACKET_SIZE];
    char full_path[MAX_PATH_SIZE];
    ssize_t file_size;
    char *end_ptr;

    //issue a pull
    snprintf(full_path,MAX_PATH_SIZE,"%s/%s",job->src.dir,job->fn);
    snprintf(buff,PACKET_SIZE,PULL_OP_STR,full_path);

    if(send_msg(buff,strlen(buff) +1,src) == -1) return -1;
    receive_msg(src,buff);

    file_size = strtol(buff,&end_ptr,10);
    if(file_size == -1) {
        receive_msg(src,report->err_msg);
        report->status = ERROR;
        report->pulled = -1;
        report->pushed = -1;
        return -1;
    }
    
    return transfer_file(src,dst,job,file_size,report);

}

// Thread handler for continuous sync job service.
void* execute_job(void* arg) {
    int src,dst;
    char full_path[MAX_PATH_SIZE];
    Job job;
    JobQueue jq = (JobQueue) arg;
    Report report;
    report.pulled = 0;
    report.pushed = 0;

    while(1) {

        if(jq_is_shutdown(jq)) return NULL;

        if(jq_dequeue(jq,&job) == -1) { // blocking till job available, thread safe
            return NULL;
        }

        if(!job.valid) continue;

        if(connect_peer(&job.src,&src) == -1 || connect_peer(&job.dst,&dst) == -1 ) {
            log_job(job,report,"Peer unreachable.",pthread_self());
            return NULL;
        } 

        pull_push(src,dst,&job,&report);
        log_job(job,report,report.err_msg,pthread_self());

        close(src);
        close(dst);
    }
}

// Spawns threads in thread_pool
int spawn_workers(pthread_t* pool,int n,JobQueue jq) {

    for(int i = 0; i < n; i++) {
        if(pthread_create(&pool[i],NULL,execute_job,(void*) jq) != 0) {
            return -1;
        }
    }

    return 0;
}

int issue_jobs(resource_id *src,resource_id *dst,JobQueue jobs) {
    Job job;
    int sock;
    char file[MAX_FILENAME_SIZE];
    char list[PACKET_SIZE];
    char *buff_ptr;

    if (connect_peer(src,&sock) == -1) return -1;
    if (get_list(list,src->dir,sock) == -1) return -1;

    buff_ptr = list;
    while(get_buff_line(file,&buff_ptr) != -1) {
        job = create_job(src,dst,file);    
        jq_enqueue(jobs,job);
    }

    return 0;
}

/* Initializes nfs manager data_strcutures based on config file given.
    Returns -1 on failure.
*/
int init_manager(const char *conf_file_path,const char *log,SyncMem sm_info,JobQueue jobs) {
    FILE* config_file;
    SyncEntry entry;
    char buff[MAX_URI_LEN*2 + 1];
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

        if(issue_jobs(&entry.src,&entry.dst,jobs)) {
            fclose(LOG);
            fclose(config_file);
            return -1;
        }
         sm_add_entry(sm_info,entry);
    }
    fclose(config_file);
    return 0;
}

int main(int argc, char **argv) {
    struct sockaddr_in manager_addr; 
    socklen_t addrlen;

    pthread_t *worker_pool;
    SyncMem watch_dirs;
    JobQueue jobs;

    size_t buff_sz;
    int port,listener_sock,console_sock;
    int max_n,connected_flag;

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

    if(watch_dirs == NULL || jobs == NULL || worker_pool == NULL) {
        perror("malloc");
        return -1;
    }
    // Initialize system by loading config entries and preparing jobs
    spawn_workers(worker_pool,max_n,jobs); 

    init_manager(cfgfile,logfile,watch_dirs,jobs);
    listener_sock = get_listener(port);
    addrlen = sizeof(manager_addr);

    while(1) {
        console_sock = accept(listener_sock,(struct sockaddr*) &manager_addr,&addrlen);
        if(console_sock < 0) {
            if (errno == EBADF || errno == EINVAL) {
                break;
            } else {
                continue;
            }
        }

        connected_flag = 0;
        while(!connected_flag) {
            connected_flag = process_command(console_sock,buff_sz,watch_dirs,jobs,worker_pool);
        }

        if(connected_flag == 1) {
            break;
        }

    }

    //Free resources
    free(worker_pool);
    sm_del(watch_dirs);
    jq_del(jobs);
    fclose(LOG);

    return 0;
}
