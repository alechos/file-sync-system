# NetSync

A distributed file synchronization system written in C. A central manager process orchestrates concurrent file transfers between remote client nodes over TCP, using a thread pool and a bounded job queue.

## Architecture

```

[Console] ──TCP──► [Manager] ──TCP──► [Client A]
│
└────TCP──────► [Client B]

```

The system has three components:

- **nfs-manager**: Central coordinator. Accepts commands from consoles, maintains a bounded job queue, and dispatches file transfer jobs to a pool of worker threads.
- **nfs-console**: Command-line interface for controlling the manager. Issues `ADD`, `CANCEL`, and `SHUTDOWN` commands over TCP.
- **nfs-client**: Endpoint node. Listens on a port for `LIST`, `PUSH`, and `PULL` operations issued by the manager's worker threads.

## How It Works

1. The manager loads a config file defining source to destination sync pairs on startup.
2. It connects to each source client, retrieves a directory listing (`LIST`), and enqueues a transfer job per file into the bounded job queue.
3. Worker threads dequeue jobs and execute them by issuing a `PULL` from the source client and a `PUSH` to the destination client. The manager acts as a relay.
4. The console can dynamically add new sync pairs (`ADD`), cancel pending transfers (`CANCEL`), or initiate a graceful shutdown (`SHUTDOWN`). Shutdown drains the queue before exiting.

## Wire Protocol

All communication uses a custom length-prefixed protocol implemented in `utils.c` via `send_msg` and `receive_msg`. Each transmission is two-phase:

1. Send the message size (4 bytes)
2. Send the message body

This ensures safe, complete, non-blocking delivery of variable-length messages over TCP. Packet size is configurable via `PACKET_SIZE` in `config.h`.

File transfers use a chunked streaming protocol with inline headers:

```

PUSH <path> <chunk_size>\n
<chunk_bytes>
PUSH <path> <chunk_size>\n

````

The final chunk uses `chunk_size=0` to signal EOF.

## Building

```bash
make all
make nfs_manager
make nfs_console
make nfs_client
make clean
````

Requires GCC and POSIX threads (`-lpthread`).

## Running

### Manager

```bash
./nfs_manager -l <logfile> -c <config_file> -n <workers> -p <port> -b <queue_size>
```

| Flag | Description                                            |
| ---- | ------------------------------------------------------ |
| `-l` | Path to manager log file                               |
| `-c` | Path to config file (source to destination sync pairs) |
| `-n` | Maximum number of worker threads                       |
| `-p` | Port to listen on for console connections              |
| `-b` | Bounded job queue capacity (slots)                     |

### Console

```bash
./nfs_console -l <logfile> -h <manager_ip> -p <manager_port>
```

| Flag | Description                      |
| ---- | -------------------------------- |
| `-l` | Path to console log file         |
| `-h` | IP address of the manager host   |
| `-p` | Port the manager is listening on |

### Client

```bash
./nfs_client -p <port>
```

| Flag | Description                               |
| ---- | ----------------------------------------- |
| `-p` | Port to listen on for incoming operations |

## Config File Format

Each line defines a sync pair:

```
<source_uri> <destination_uri>
```

URIs follow the format `/<directory>@<host_ip>:<port>`. The leading `/` is required, but the path is not an absolute system path. It is resolved relative to the working directory of the `nfs_client` process. Each client has its own root, and all paths are interpreted from there. Source and destination directories must not overlap. No entry may be a subdirectory of another.

Example:

```
/data@192.168.1.10:8080 /backup@192.168.1.20:8081
```

## Console Commands

| Command                   | Description                                                                                                                                                                                           |
| ------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `add <src_uri> <dst_uri>` | Begin syncing a new source to destination directory pair. If the pair is already queued, the manager skips it and replies "Already in queue". Only flat directories are supported. No subdirectories. |
| `cancel <src_uri>`        | Cancel pending transfers for a source directory                                                                                                                                                       |
| `shutdown`                | Drain the job queue and shut down the manager gracefully                                                                                                                                              |

## Logging

Both the manager and console maintain separate log files.

### Manager Log (`-l` flag)

Worker sync operations:

```
[TIMESTAMP] [SOURCE_FILE@HOST:PORT] [TARGET_FILE@HOST:PORT] [THREAD_ID] [OPERATION] [RESULT] [DETAILS]
```

Example:

```
[2025-02-10 10:00:01] [/dir1/file1@1.2.3.4:8080] [/dir2/file1@4.5.6.7:8090] [1234] [PULL] [SUCCESS] [10 bytes pulled]
[2025-02-10 10:00:02] [/dir1/file1@1.2.3.4:8080] [/dir2/file1@4.5.6.7:8090] [1234] [PUSH] [SUCCESS] [10 bytes pushed]
```

ADD command output (also printed to stdout and sent to console):

```
[2025-02-10 10:00:01] Added file: /dir1/file1@1.2.3.4:8080 -> /dir2/file1@4.5.6.7:8090
```

SHUTDOWN sequence:

```
[2025-02-10 10:23:02] Shutting down manager...
[2025-02-10 10:23:02] Waiting for all active workers to finish.
[2025-02-10 10:23:03] Processing remaining queued tasks.
[2025-02-10 10:24:01] Manager shutdown complete.
```

Note: Log files are cleared on each startup. CANCEL is not reliably logged.

### Console Log (`-l` flag)

Records commands issued by the user:

```
[2025-02-10 10:00:01] Command add /dir1@1.2.3.4:8080 -> /dir2@4.5.6.7:8090
[2025-02-10 10:23:01] Command cancel /dir1
[2025-02-10 10:23:01] Command shutdown
```

All timestamps use the format `%Y-%m-%d %H:%M:%S`.

## Performance

Tested on localhost with an artificial 10 ms network delay (`tc netem`) and 8 worker threads. Transferred 1,000 files (about 100 MB total) in about 15 seconds (about 6.7 MB/s).

## Implementation Notes

* **Job queue**: Bounded, thread-safe queue using mutexes and condition variables. Supports mid-queue cancellation and graceful shutdown drain.
* **Worker threads**: Pool of N threads, each blocking on `jq_dequeue` until a job is available. Threads exit cleanly on shutdown signal.
* **Sync memory**: Tracks active sync entries to prevent duplicate jobs and validate new `ADD` commands against existing destinations.
* **Overwrite behavior**: Existing files at the destination are always overwritten without timestamp checking.
* **Error handling**: Transfer errors are propagated back through the relay using inline `MSG_ERR` sentinel messages, with `strerror_r` for thread-safe error string generation.
* **Low-level I/O**: All file operations use POSIX syscalls (`open`, `read`, `write`, `close`) with no buffered stdio.
