# **NetworkFile System**

 ## ***Usage***
 ---

 To compile all components of the Network File system run `make all` 

***Nfs-Manager***
*The central program of the system, coordinating client communication and logging.*

To run nfs_manager:
* run `make nfs_manager`
* run `./nfs_manager -l <manager_logfile> -c <config_file> -n <worker_limit> -p <port_number> -b <bufferSize>`

 `<<manager_logfile>>` is the path to the file the manager will write its logs into.
 `<config_file>`  is the path to the fss_manager configuration file.
 `<worker_limit>` is the maximum amount of worker threads the manager will deploy.
 `<port_number>`  is the port the manager will listen to for incoming console connections.
 `<bufferSize>`  is the maximum count of slots the managers joq queue will hold.

***Nfs-Console***
*Console providing an interface to the nfs-manager*
To run fss_console:
* run `make nfs_console`
* run `./nfs_console -l <console-logfile> -h <host_IP> -p <host_port>`

`<console-logfile>` is the path to the file the console will write its logs into.
`<host_IP>` is the IP address of the host running the manager.
`<host_port>` is the port the console connects to with the intent of communicating with the manager

 ***Nfs-Client***
 *Client program, perpetually listening to a given port for a LIST,PUSH or PULL command.*
 *Constitute the endpoints of the system.*
To run nfs_client:
* run `make nfs_client`
* run `./nfs_client -p <port_number>`

`<port_number>` refers to the port the client will listen to for incoming operation commands.

# **Notes**

Communication through sockets happen using the `send_msg` and `receive_msg` functions
defined in `utils.c`. Their purpose is ensuring safe, complete and non blocking transmission
of variable sized messages. Communication using these functions happens in 2 phases internally.
`send_msg` sends the message size in phase 1 and the actual message in phase 2. `receive_msg`
reads the message size first and writes the message read in a buffer.

The buffer size used by these functions is set to `PACKET_SIZE`, defined in `config.h`.
