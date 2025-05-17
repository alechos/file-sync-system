#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <signal.h>
#include "utils.h"
#include "exec_report.h"
#include "config.h"
#include "job.h"

//TODO  handle buffer overflows
//      more robust error handling
//      file locking
typedef struct err_buff {
    char buffer[BUFSIZ];
    size_t offset;    
}  ErrBuff;

int write_err(ErrBuff *buff,char *str,char *fn) {
    size_t new_offset,max_sz;
    char* ptr;

    ptr = buff->buffer + buff->offset;
    max_sz = BUFSIZ - buff->offset;
    
    if (fn) {
        new_offset =  snprintf(ptr,max_sz,"File %s: %s\n",fn,str);
    } else {
        new_offset =  snprintf(ptr,max_sz,"Error: %s\n",str);
    }
        if(new_offset < 0 || new_offset >= max_sz) {
        return -1;
    }
    buff->offset+=new_offset;
    return 0;
}

int log_err(Report *report,ErrBuff *buff,char* filename) {
    report->errors++;
    report->skipped++;
    return write_err(buff,strerror(errno),filename);
}

int get_file_path(char *dir,char *filename,char *full_path) {
    int result;
    result = snprintf(full_path,MAX_PATH_SIZE,"%s/%s",dir,filename);
    if(result < 0 || result >= MAX_PATH_SIZE) {
        return -1;
    }
    return 0;
}

/* Copy file filename from src to dst and updates report and err_buff.
    Returns -1 on failure.
*/
int copy_file(int src,int dst,char* filename,Report *report,ErrBuff *err_buff) {
    char buffer[BUFSIZ];
    char* ptr = buffer;
    ssize_t bytes_w = 0,total_r = 0;

    // Attempt to read BUFSIZ sized chunks into buffer until EOF
    while((total_r = read(src,buffer,BUFSIZ)) != 0) {
        if(total_r == -1) { 
            if (errno == EINTR) {
                continue;
            } else {
                //A fatal error happened, report it and return
                log_err(report,err_buff,filename);
                close(src);
                close(dst);
                return -1;
            }
        }
        // Reset pointer to start of buffer
        ptr = buffer;

        // write the chunk read in dst until total read bytes have been written
        while(total_r > 0) {
            bytes_w = write(dst,ptr,total_r);  
            if ((bytes_w < 0) && (errno != EINTR) ) { 
                log_err(report,err_buff,filename);
                close(src);
                close(dst);
                return -1;
            
            }
            // Iterate
            total_r -= bytes_w;
            ptr+=bytes_w;
        } 
    }
    return 0;
}
/* Delete file filename in directory target
    Returns -1 on failure*/
int delete(char *target,char *filename,Report *report,ErrBuff *buff) {
    char full_tar[MAX_PATH_SIZE];
    int result;

    get_file_path(target,filename,full_tar);
    result = unlink(full_tar);
    if(result) {
        log_err(report,buff,filename);
    } else {
        report->deleted++;
    }
    return result;
}

/* Perform add operation, add file with name filename from source to dst, also report errors.
Return -1 on failure.*/
int add(char *source,char *target,char *filename,Report *report,ErrBuff *err_buff) {
    char full_src[MAX_PATH_SIZE],full_tar[MAX_PATH_SIZE];
    int src,dst;
    get_file_path(source,filename,full_src);
    get_file_path(target,filename,full_tar);

    src = open(full_src,O_RDONLY);
    if(src == -1) {
        log_err(report,err_buff,filename);
        return -1;
    }

    dst = open(full_tar,O_WRONLY | O_TRUNC | O_CREAT,0666);
    if(dst == -1) {
        log_err(report,err_buff,filename);
        close(src);
        return -1;
    }

    if(!copy_file(src,dst,filename,report,err_buff)) {
        report->copied++;
    }
    close(src);
    close(dst);
    return 0;
}

/* Copies each file in directory with name source to destination.
    Return -1.
*/
int sync_dir(char* source,char *destination,Report *report,ErrBuff *buff) {
    DIR *src_dir,*dst_dir;
    struct dirent* file;
    int ret = 0;

    src_dir = opendir(source);
    dst_dir = opendir(destination);

    // One of the directories could not be opened
    if (!src_dir || !dst_dir) {
        if(src_dir) closedir(src_dir);
        if(dst_dir) closedir(dst_dir);
        write_err(buff,strerror(errno),NULL);
        report->errors+=1;
        return -1;
    }

    // For each file in source add it to destination
    while((file = readdir(src_dir)) != NULL) {
        if (file->d_name[0] == '.') { //skip . , .. directories and hidden files
            continue;
        }
        if(add(source,destination,file->d_name,report,buff)) {
            ret = -1;
        }
    } 

    closedir(src_dir);
    closedir(dst_dir);
    return ret;
}

/* Convert string to OPERATION.
    Returns INVALID_OP in case of invalid string.*/
OPERATION str_to_op(char *str) {
    OPERATION op;
    if(!strcmp("FULL",str)) {
        op = FULL;
    } else if(!strcmp("ADDED",str)) {
        op = ADDED;
    } else if(!strcmp("MODIFIED",str)) {
        op = MODIFIED;
    } else if(!strcmp("DELETED",str)) {
        op = DELETED;
    } else {
        op = INVALID_OP;
    }

    return op;
}

int main(int argc,char **argv) {
    char *source;
    char *target;
    char *file_name;
    ErrBuff buff;
    OPERATION operation; 

    Report report;
    report.status = SUCCESS;
    buff.offset = 0;
    memset(&report,0,sizeof(report));
    if(argc != 5) {
        exit(EXIT_FAILURE);
    }

    source = argv[1];
    target = argv[2];
    file_name = argv[3];
    operation = str_to_op(argv[4]);

    switch (operation)
    {
    case FULL:

        if(sync_dir(source,target,&report,&buff)) {
            if(report.copied > 0) {
                report.status = PARTIAL;
            } else {
                report.status = ERROR;
            }
        }
        break;
    case MODIFIED:
    if(add(source,target,file_name,&report,&buff)) {
        report.status = ERROR;
    } 
    break;
    case ADDED:
        if(add(source,target,file_name,&report,&buff)) {
            report.status = ERROR;
        } 
        break;
    case DELETED:
        if(delete(target,file_name,&report,&buff)) {
            report.status = ERROR;
        }
        break;
    default:
        break;
    }

    // Write report and errbuff to pipe
    report.err_len = buff.offset;
    if (write_buff((char*)&report,sizeof(report),STDOUT_FILENO) == -1) perror("Exec_Report error");
    if (write_buff(buff.buffer,buff.offset,STDOUT_FILENO) == -1) perror("Exec_Report error");

    exit(report.status);
}