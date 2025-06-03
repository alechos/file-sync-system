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


#define WATCH_OPTS (IN_MOVED_TO|IN_DELETE | IN_CLOSE_WRITE)

FILE *LOG = NULL;

int generate_msg(char*,char*,char*,char*,int,ssize_t);
int log_and_print(char*,size_t);
int broadcast(int,char*,size_t);

/* Initializes fss manager data_strcutures based on config file given.
    Returns -1 on failure.
*/
int init_manager(const char *conf_file_path,const char *log,SyncMem sm_info,JobQueue jobs,int *fd) {
    FILE* config_file;
    SyncEntry entry;
    Job sync_job;
    int wd;
    char buff[MAX_PATH_SIZE*2 + 1],msg_buff[MAX_MSG_SIZE];
    char src[MAX_PATH_SIZE],dst[MAX_PATH_SIZE];

    config_file = fopen(conf_file_path,"r");   
    LOG = fopen(log,"w");

    *fd = inotify_init1(IN_NONBLOCK);
    if (*fd == -1 || !(config_file)) {
        perror("init_manager: ");
        return -1;
    }

    // Iterate through configuration file lines
    while(fgets(buff,MAX_PATH_SIZE*2 + 1,config_file)) {
        buff[strcspn(buff, "\n")] = 0;
        // Extract source and destination paths
        snprintf(src,MAX_PATH_SIZE,"%s",strtok(buff," "));
        snprintf(dst,MAX_PATH_SIZE,"%s",strtok(NULL," "));

        // Begin monitoring for each souce directory
        if( (wd = inotify_add_watch(*fd,src,WATCH_OPTS)) != -1 ) {
            sync_job = create_job(src,dst,"ALL",FULL);
            entry = create_sync_entry(src,dst,time(NULL),ACTIVE,wd,0);
            
            // Log entries
            generate_msg(msg_buff,ADDED_DIR_STR,src,dst,-1,MAX_MSG_SIZE);
            log_and_print(msg_buff,strlen(msg_buff) + 1);
           
            generate_msg(msg_buff,MON_STARTED_STR,src,dst,-1,MAX_MSG_SIZE);
            log_and_print(msg_buff,strlen(msg_buff) + 1);

            // Catalogue the monitoring in sm_info and enque corresponding sync job
            sm_add_entry(sm_info,entry);
            jq_enqueue(jobs,sync_job);
        } else {
            return -1;
        }
    }
    fclose(config_file);
    return 0;
}


/*  Parses program aguments.
    Returns -1 on failure.
*/
int parse_args(char **logfile,char **cfgfile,int *max_n,int argc,char **argv) {
    int opt;
    while((opt = getopt(argc,argv,"l:c:n:"))!= -1) {
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
        );
        return -1;
    } 

    return 0;
}
/* Returns string describing operation op in name buffer.
   Buffer must be of at least size 9.
   Returns -1 on failure.
*/
int get_op_name(OPERATION op,char *name) { //change maybe
    switch (op)
    {
    case FULL:
        strcpy(name,"FULL");
        break;
    case ADDED:
        strcpy(name,"ADDED");
        break;
    case MODIFIED:
        strcpy(name,"MODIFIED");
        break;   
    case DELETED:
        strcpy(name,"DELETED");
        break;
    default:
        strcpy(name,"INVALID");
        return -1;
        break;
    }
    return 0;
}

/*
    Returns operation corresponding to inotify event mask.
    In case of failure INVALID_OP is returned.
*/
OPERATION parse_op(char *path,uint32_t mask) {
    struct stat info;
    
    //file finished being written or was moved to path
    if (mask&IN_CLOSE_WRITE || mask&IN_MOVED_TO) {
        if(stat(path,&info) == 0) { 
            //if file exists in target its been modified
            return MODIFIED; 
        } else {
            return ADDED;
        }        
    } else if(mask&IN_DELETE) {
        return DELETED ;
    }
    return INVALID_OP;
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

/* Spawns a new worker proccess,adds it's entry in the given WorkerList
   and establishes a unidirectional pipe from the worker to the manager.
   Returns -1 on failure.
   */
int spawn(Job job,WorkerList wl) {
    Worker worker;
    pid_t pid;
    char name[10];
    int fds[2];

    // Active workers at capacity.
    if(wl->capacity == wl->size) {
        return -1;
    }

    // Pipe failed
    if(pipe(fds) == -1) {
        perror("pipe: ");
        return -1;
    }
    get_op_name(job.op,name);

    // Fork failed
    if((pid = fork()) == -1) {
        perror("fork :");
        return -1;
    }  

    if(pid == 0) {
        //Child
        close(fds[0]);
        dup2(fds[1],1);
        close(fds[1]);
        execl("worker","worker",job.sd,job.td,job.fn,name,NULL);
        return -1;
    } else {
        //Parent
        close(fds[1]);
        worker = create_worker(pid,fds[0],job);
        //Worker "inherits" tracking from job
        if(job.tracked) worker.tracked = true;
        wl_add(wl,worker);
    }
    return 0;
}

/* Issue a new job, enqueing it if active children at capacity.
   Returns -1 on failure.
*/
int issue_job(Job job,WorkerList wl,JobQueue jobs,SyncMem sm) {
    SyncEntry entry;
    if(wl->size != wl->capacity) {
        sm_get_entry(sm,&entry,job.sd);
        entry.sync_timestamp = time(NULL); // Updating timestamp.
        sm_add_entry(sm,entry);
        return spawn(job,wl);
    } else {
        return jq_enqueue(jobs,job);
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

/* Handles worker exit by reading its exec Report into ```*report``` and any error
   messages in a buffer pointed to by ```*err_msg```.
   WARNING: caller must free ```*err_msg```.
*/
int handle_worker_exit(Worker worker,SyncMem sm_info,Report *report,char **err_msg) {
    char *rep_buff = (char*) report;
    SyncEntry entry;
    Job cmplt_job;

    if(((read_buff(rep_buff,sizeof(Report),worker.fd) > 0)) ) {
        *report = *((Report*) rep_buff);  // Cast read bytes to Report type

        *err_msg = malloc(report->err_len);
        if(report->err_len != 0) {
            read_buff(*err_msg,report->err_len,worker.fd);
            (*err_msg)[report->err_len - 1] = 0; // removing new line

            cmplt_job = worker.assigned_job;
            sm_get_entry(sm_info,&entry,cmplt_job.sd);
            entry.error_count+= report->errors;
            sm_add_entry(sm_info,entry);
        }
        log_job(worker.assigned_job,*report,*err_msg,worker.pid);
        return 0;
    }
    return -1;
}

/*Broadcast tracked sync result to stdout,
  console (whose fd is passed in ```out```) and log.*/
int broadcast_sync_result(Worker worker, Report report,SyncMem sm_info,int out) {
    char msg[MAX_MSG_SIZE] = {0};
    SyncEntry entry;
    Job cmplt_job = worker.assigned_job;

    sm_get_entry(sm_info,&entry,cmplt_job.sd);
    
    generate_msg(msg,SYNC_DONE_STR,entry.sd,entry.td,report.errors,MAX_MSG_SIZE);
    if (broadcast(out,msg,strlen(msg)+1) ==-1) return -1;
    if (send_msg(MSG_END,strlen(MSG_END)+1,out) == -1) return -1;

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
    if(!strcmp("status",arg)) return STATE;
    if(!strcmp("cancel",arg)) return CANCEL;
    if(!strcmp("sync",arg)) return SYNC;
    if(!strcmp("shutdown",arg)) return SHUTDOWN;
    return INVALID_COM;
}

/* Converts command to string and stores in line.
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
        snprintf(command->source,MAX_PATH_SIZE,"%s",arg);
    }

    if(command->com == ADD) {
        arg = strtok(NULL," ");
        snprintf(command->destination,MAX_PATH_SIZE,"%s",arg);
    }

    return 0;

}

/*Handler for add command by console. Begins monitoring for a new or 
stopped direcotry and executes a full sync job if not already in queue.*/
int console_add(int out,Command com,JobQueue jobs, SyncMem sm_info,WorkerList wl,int event_fd) {
    SyncEntry entry;
    Job sync_job;
    time_t time_stamp;
    int wd,entry_exists;
    char *src = com.source;
    char *dst = com.destination;
    char msg_buff[MAX_MSG_SIZE] = {0};

    entry_exists = (sm_get_entry(sm_info,&entry,com.source) != -1);
    time_stamp = (entry_exists) ? entry.sync_timestamp : time(NULL); 
    
    if (entry_exists && strcmp(entry.td,dst) ) { // New destination doesn't match previous
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
int process_command(int in,int out,SyncMem sm,JobQueue jobs,WorkerList wl,int watch_fd) {
    char buff[MAX_LINE];
    Command command;
    ssize_t size;
    // Receive command from console
    if ((size = receive_msg(in,buff)) > 0) {
        buff[size] = 0;
        parse_command(buff,&command);
    }

    switch (command.com) {
    case ADD:
        return console_add(out,command,jobs,sm,wl,watch_fd);
    case CANCEL:
        return console_cancel(out,command,sm,watch_fd);
    case SHUTDOWN:
        // If shutdown succesfully return 1, signify shutdown
        if(!console_shutdown(out,sm,wl,jobs)) return 1;
    default:
        send_msg("Invalid Command\n",17 + 1,out);
        send_msg(MSG_END,strlen(MSG_END) + 1,out);
        break;
    }

    return -1;
}

pthread_t* create_pool(int n,JobQueue jq) {
    pthread_t *pool = malloc(n*sizeof(pthread_t));

    for(int i = 0; i < n; i++) {
        pthread_create(&pool[i],NULL,handle_job,(void*) jq);
    }
}

int main(int argc, char **argv) {
    pthread_t *worker_pool;
    SyncMem watch_dirs;
    JobQueue jobs;
    Job new_job;
    int max_n,ret;
    char *logfile = NULL,*cfgfile =NULL;
    max_n = 5;

    if(parse_args(&logfile,&cfgfile,&max_n,argc,argv) < 0) {
        exit(1);
    }

    // Create sync_info struct and Job queue stucts
    watch_dirs = sm_create();
    jobs = jq_create();
    worker_pool = create_pool(max_n);
    // Initialize system by loading config entries and preparing jobs
    init_manager(cfgfile,logfile,watch_dirs,jobs,???);

    // Spawn config jobs queued by ini_manager CHANGE TO THREAD POOL
    for(int i = 0; i < max_n && !jq_is_empty(jobs); i++) {
        if(jq_dequeue(jobs,&new_job) != -1) {
            spawn(new_job,workers);
        }
    }

    while(1) { 

        // A new console command has been received

        //This is the first communication with the console,initialize...
        //if(process_command(fds[0].fd,fifos[1],watch_dirs,jobs,workers,watch_fd) == 1) {
        //    break; //shutdwon has been initiated
       // }
    }

    //Free resources
    wl_free(workers);
    sm_del(watch_dirs);
    jq_del(jobs);
    fclose(LOG);

    return 0;
}
