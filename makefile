CC = gcc
CFLAGS = -Wall -g3 -Iinclude -Iinclude/sync -Iinclude/job -Iinclude/worker

SRCDIR = src
OBJDIR = obj
INCLUDEDIR = include

SRCS_FSS_CON = $(SRCDIR)/fss_console.c
SRCS_SYNC_WORKER = $(SRCDIR)/sync_worker.c
SRCS_FSS_MAN = $(SRCDIR)/fss_manager.c 
SRCS_QUEUE = $(SRCDIR)/job/job.c $(SRCDIR)/job/queue.c
SRCS_SM = $(SRCDIR)/sync/sync.c $(SRCDIR)/sync/sync_mem.c 
SRCS_WORKERS = $(SRCDIR)/worker/worker.c $(SRCDIR)/worker/worker_list.c 
SRCS_UTILS = $(SRCDIR)/utils.c

OBJS_SYNC_WORKER = $(SRCS_SYNC_WORKER:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
OBJS_FSS_MAN = $(SRCS_FSS_MAN:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
OBJS_FSS_CON = $(SRCS_FSS_CON:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
OBJS_QUEUE = $(SRCS_QUEUE:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
OBJS_SM = $(SRCS_SM:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
OBJS_WORKERS = $(SRCS_WORKERS:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
OBJS_UTILS = $(SRCS_UTILS:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

TARGET_FSS_CON = fss_console
TARGET_SYNC_WORKER = worker
TARGET_FSS_MAN = fss_manager

$(TARGET_SYNC_WORKER): $(OBJS_SYNC_WORKER) $(OBJS_UTILS)
	$(CC) $(OBJS_SYNC_WORKER) $(OBJS_UTILS) -o $(TARGET_SYNC_WORKER)

$(TARGET_FSS_CON): $(OBJS_UTILS) $(OBJS_FSS_CON) 
	$(CC) $(OBJS_UTILS) $(OBJS_FSS_CON)  -o $(TARGET_FSS_CON)


$(TARGET_FSS_MAN):  $(OBJS_QUEUE) $(OBJS_SM) $(OBJS_WORKERS) $(OBJS_FSS_MAN) $(OBJS_UTILS) $(TARGET_SYNC_WORKER)
	$(CC) $(OBJS_QUEUE) $(OBJS_SM) $(OBJS_FSS_MAN) $(OBJS_WORKERS) $(OBJS_UTILS) -o $(TARGET_FSS_MAN)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)/$(dir $*) 
	$(CC) $(CFLAGS) -c $< -o $@  

all: $(OBJDIR) $(TARGET_SYNC_WORKER) $(TARGET_FSS_CON) $(TARGET_FSS_MAN) 
clean:
	rm -rf $(OBJDIR) $(TARGET_SYNC_WORKER) $(TARGET_FSS_CON) $(TARGET_FSS_MAN) 

.PHONY: all clean $(OBJDIR) $(TARGET_SYNC_WORKER) $(TARGET_FSS_CON) $(TARGET_FSS_MAN) 

