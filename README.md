# **FileSync System**

The objective of this project is the implementation of a file synchronization system
involving real-time monitoring of directories and their syncrhonization to mapped backup
directories.

Communication with the system is available to the user through the fss-console interface, while intergrated logging for both the main system (ie. fss-manager) and the console is also present.

A method for executing miscellaneous tasks like report creation and backup/log purging is provided as well through the fss-script.

 ## ***FileSync System Design***
 ---
The central process of the system is the fss-manager. The system is initialized by the execution of the fss-manager. 

**Fss-manager initialization**
The manager initialization sequence is as follows:

1. Establishes required channels for console communication and logging
2. Continues with parsing the provided config file
3. Begins the synchronization of each "src_path dst_path" pair read from the config file
4. Ends by entering the programs main loop, polling for any changes in the directories it's monitoring or a new command received from the console

For the execution of assigned syncrhonization jobs the manager spawns worker children proccesses. 

**Synchronization Worker**

Each worker is responsible for executing a signle synchronization job and communicates it's status to the manager on exit through a pipe. 

Jobs are created:
* during the manager initialization stage for each directory - target pair.
* due to a modification in a monitored directory .
* due to a user request via a console issued command. 

When a worker is done the manager is notified through a signal and handles the worker exit by reaping any worker that's done. The job status is read from the manager through the pipe to the worker and then logged.

**Fss-console**

The fss-console is the sole interface with the fss-manager while the manager is running. Inter proccess communcation between the manager and the console is achieved through the use of named pipes. The user issues a valid command through the console which is send through the input pipe to the manager, the manager executes the command and returns the results to the console through the output pipe. Manager must be open for fss-console to open successfully.

                        Sends                                 Receives
                        command                               command
                         _________                            _________
                        |         | ======= FSS-IN ======= > |         |
                        | CONSOLE |                          | MANAGER |
                        |         | <======= FSS-OUT ======  |         |
                        -----------                          -----------

                        Waits for                             Sends command
                        result                                result

 **Fss-Script**

The main functions or commands of the fss-script are as follows:
* **listAll**, lists all directories in passed log file.
* **listMonitored**, lists only directories logged as monitored currently or at exit.
* **listStopped**, lists only directories logged as stopped currently or at exit.
* **purge**, purges backup directory or log file passed.

 ## ***Usage***
 ---

 To compile all components of the FileSync system run `make all` 

***Fss-Manager***
To run fss_manager:
* run `make fss_manager`
* run `./fss_manager -c <config_file> -l <log_file>`

 `<config_file>` is the path to the fss_manager configuration file.
 `<log_file>` is the path to the file the manager will write its logs into.

***Fss-Console***
To run fss_console:
* run `make fss_console`
* run `./fss_console -l <log_file>`

`<log_file>` is the path to the file the console will write its logs into.

 ***Fss-Script***
To run the fss-script:
* run `./fss_script.sh -p <path_name> -c <command>`

`<path_name>` is the path to the log file to analyze or the target to purge.
`<command>` is the requested command.

