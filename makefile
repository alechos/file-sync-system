CC = gcc
CFLAGS = -Wall -g3 -pthread -Iinclude -Iinclude/sync -Iinclude/job -Iinclude/worker -Iinclude/client

SRCDIR = src
OBJDIR = obj
INCLUDEDIR = include

SRCS_CLIENT = $(SRCDIR)/client.c
SRCS_FSS_CON = $(SRCDIR)/fss_console.c
SRCS_FSS_MAN = $(SRCDIR)/fss_manager.c 
SRCS_QUEUE = $(SRCDIR)/job/job.c $(SRCDIR)/job/queue.c
SRCS_SM = $(SRCDIR)/sync/sync.c $(SRCDIR)/sync/sync_mem.c 
SRCS_UTILS = $(SRCDIR)/utils.c

OBJS_CLIENT = $(SRCS_CLIENT:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
OBJS_FSS_MAN = $(SRCS_FSS_MAN:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
OBJS_FSS_CON = $(SRCS_FSS_CON:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
OBJS_QUEUE = $(SRCS_QUEUE:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
OBJS_SM = $(SRCS_SM:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
OBJS_UTILS = $(SRCS_UTILS:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

TARGET_CLIENT = client
TARGET_FSS_CON = fss_console
TARGET_FSS_MAN = fss_manager


$(TARGET_CLIENT): $(OBJS_UTILS) $(OBJS_CLIENT)
	$(CC) $(OBJS_CLIENT) $(OBJS_UTILS) -o $(TARGET_CLIENT)

$(TARGET_FSS_CON): $(OBJS_UTILS) $(OBJS_FSS_CON) 
	$(CC) $(OBJS_UTILS) $(OBJS_FSS_CON)  -o $(TARGET_FSS_CON)

$(TARGET_FSS_MAN):  $(OBJS_QUEUE) $(OBJS_SM) $(OBJS_FSS_MAN) $(OBJS_UTILS)
	$(CC) $(OBJS_QUEUE) $(OBJS_SM) $(OBJS_FSS_MAN) $(OBJS_UTILS) -o $(TARGET_FSS_MAN)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)/$(dir $*) 
	$(CC) $(CFLAGS) -c $< -o $@  

all: $(OBJDIR) $(TARGET_FSS_CON) $(TARGET_FSS_MAN) $(TARGET_CLIENT)
clean:
	rm -rf $(OBJDIR) $(TARGET_FSS_CON) $(TARGET_FSS_MAN) $(TARGET_CLIENT)

.PHONY: all clean $(OBJDIR) $(TARGET_SYNC_WORKER) $(TARGET_FSS_CON) $(TARGET_FSS_MAN) 

