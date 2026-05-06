CC = gcc
CFLAGS = -g3 -pthread -Iinclude -Iinclude/client -Iinclude/sync -Iinclude/job
LDFLAGS = -pthread

SRCDIR = src
OBJDIR = obj

SRCS_NFS_CLIENT = $(SRCDIR)/nfs_client.c
SRCS_NFS_CON = $(SRCDIR)/nfs_console.c
SRCS_NFS_MAN = $(SRCDIR)/nfs_manager.c 
SRCS_QUEUE = $(SRCDIR)/job/job.c $(SRCDIR)/job/queue.c
SRCS_SM = $(SRCDIR)/sync/sync.c $(SRCDIR)/sync/sync_mem.c 
SRCS_UTILS = $(SRCDIR)/utils.c

OBJS_NFS_CLIENT = $(SRCS_NFS_CLIENT:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
OBJS_NFS_MAN = $(SRCS_NFS_MAN:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
OBJS_NFS_CON = $(SRCS_NFS_CON:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
OBJS_QUEUE = $(SRCS_QUEUE:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
OBJS_SM = $(SRCS_SM:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
OBJS_UTILS = $(SRCS_UTILS:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

TARGET_NFS_CLIENT = nfs_client
TARGET_NFS_CON = nfs_console
TARGET_NFS_MAN = nfs_manager

$(TARGET_NFS_CLIENT): $(OBJS_NFS_CLIENT) $(OBJS_UTILS)
	$(CC) $^ -o $@ $(LDFLAGS)

$(TARGET_NFS_CON): $(OBJS_NFS_CON) $(OBJS_UTILS)
	$(CC) $^ -o $@ $(LDFLAGS)

$(TARGET_NFS_MAN): $(OBJS_QUEUE) $(OBJS_SM) $(OBJS_NFS_MAN) $(OBJS_UTILS)
	$(CC) $^ -o $@ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)/$(dir $*)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

all: $(TARGET_NFS_CON) $(TARGET_NFS_MAN) $(TARGET_NFS_CLIENT)

clean:
	rm -rf $(OBJDIR) $(TARGET_NFS_CLIENT) $(TARGET_NFS_CON) $(TARGET_NFS_MAN)

-include $(OBJS_NFS_CLIENT:.o=.d) \
         $(OBJS_NFS_CON:.o=.d) \
         $(OBJS_NFS_MAN:.o=.d) \
         $(OBJS_QUEUE:.o=.d) \
         $(OBJS_SM:.o=.d) \
         $(OBJS_UTILS:.o=.d)

.PHONY: all clean