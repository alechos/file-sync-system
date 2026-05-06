#ifndef CONFIG_H
#define CONFIG_H

#define MAX_HOST_SIZE 64
#define MAX_PORT_SIZE 16
#define MAX_PATH_SIZE 512
#define MAX_URI_LEN  (MAX_HOST_SIZE + MAX_PORT_SIZE + MAX_PATH_SIZE)

#define MAX_MSG_SIZE 512
#define MAX_FILENAME_SIZE 255 
#define PACKET_SIZE 65536
#define MAX_LINE 512


#define MAX_BACKLOG 10
#define LIST_OP_STR "LIST %s"
#define PUSH_OP_STR "PUSH %s %d "
#define PULL_OP_STR "PULL %s "
#define FSS_IN "tmp/fss_in"
#define FSS_OUT "tmp/fss_out"
#define MSG_END "--END_OF_MSG--"
#define MSG_ERR "--MSG_ERR--"


#endif
