/* My Shell
 * OS Project 2
 * Noah Yoshida
 * Supports commands:
 *  start
 *  wait
 *  run
 *  kill
 *  stop
 *  continue
 */

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "debug.h"

#include "colors.h"

char PROMPT[] = "$ ";

void prompt(char * COLOR) {
  /* Prints the prompt */
    char buff[BUFSIZ];
    getcwd(buff, BUFSIZ);
    printf("%s%s%s\n%s%s%s", KBLU, buff, KNRM, COLOR, PROMPT, KNRM);
    fflush(stdout);
}

int get_command(char * string) {
    /* Returns the code for the command or prints error if none is found */
    int i;
    int n = 6;
    char * commands[6] = { "start", "wait", "kill", "stop", "continue", "exit"};
    for (i = 0; i < n; i ++) {
        // Is it the command?
        if (strcmp(commands[i], string) == 0) return i;
    }

    return 6;

    // Bad command
    fprintf(stderr, "[ERROR] Unknown command: %s\n", string);
    return -1;
}

void handle_signals(int command, int pid) {
    /* Send the signal specified by command to process pid */
    int sig = -1;
    char * descriptor;

    // Handle the different signals
    if (command == 3) {
        // Kill
        sig = SIGKILL;
        descriptor = "killed";
    } else if (command == 4) {
        // Stop
        sig = SIGSTOP;
        descriptor = "stopped";
    } else if (command == 5) {
        // Continue
        sig = SIGCONT;
        descriptor = "continued";
    } else {
        // Error
        fprintf(stderr, "[ERROR] Invalid command given\n");
        return;
    }

    // Send signal and do error checking
    if (kill((pid_t) pid, sig) < 0) {
        fprintf(stderr, "[ERROR] Error in signal to pid %d: %s\n", pid, strerror(errno));
    } else {
        printf("[INFO] process %d %s\n", pid, descriptor);
    }
}

int handle_process_exit(int pid, int status) {
    /* Handles a successful wait call */
    if (WIFEXITED(status)) {
#ifdef DEBUG
        printf("[INFO] Process %d exited normally with status %d\n", pid, WEXITSTATUS(status));
#endif
        return status;
    } else if (WIFSIGNALED(status)) {
#ifdef DEBUG
        printf("[INFO] Process %d exited with signal %d\n", pid, WTERMSIG(status));
#endif
        return status;
    } else if (WIFSTOPPED(status)) {
#ifdef DEBUG
        printf("[INFO] Process %d stopped with signal %d\n", pid, WSTOPSIG(status));
#endif
        return status;
    } else {
        fprintf(stderr, "[ERROR] Error in processing child process %d\n", pid);
        return status;
    }
    return status;
}

void swap_fds(char * fname, int fd) {
    /* Safely swaps the filename into the file descriptor fd */
    // Open the fd differntly if the target fd is stdin or stdout
    int newFd = (fd == 0) ? open(fname, O_RDONLY) : open(fname, O_WRONLY | O_CREAT, 0666);

    if (newFd < 0) {
        fprintf(stderr, "[ERROR] %s\n", strerror(errno));
        close(newFd);
    } else {
        if (dup2(newFd, fd) < 0)
            fprintf(stderr, "[ERROR] Error in opening file %s: %s\n", fname, strerror(errno));
        else
            close(newFd);
    }
}

void handle_file_redirection(char * inputFile, char * outputFile) {
    // See if we need to redirect stdin
    if (inputFile != NULL) swap_fds(inputFile, 0);
    if (outputFile != NULL) swap_fds(outputFile, 1);
}

void start_process(char * args[], char * inputFile, char * outputFile) {
    /* Starts a child process */
    // Fork a child process
    int _pipe[2];
    pipe(_pipe);
    pid_t pid = fork();

    // In parent, print off the process id of child
#ifdef DEBUG
    if (pid > 0) printf("[INFO] Process %d started\n", pid);
#endif

    // Handle fork
    if (pid < 0) {
        // Fork error detected in parent
        fprintf(stderr, "[ERROR] Error forking child process: %s\n", strerror(errno));
    } else if (pid == 0) {
        // In child process...
        close(_pipe[1]);
        handle_file_redirection(inputFile, outputFile);

        if (execvp(args[0], args) < 0) {
        // Error in execvp
        fprintf(stderr, "[ERROR] Error in executing program %s: %s\n", args[0], strerror(errno));
        // Child exit with failure
        exit(EXIT_FAILURE);
        }
    }
}

void use_pipe(int fd, int * pip) {
    if (fd == 1)
        close(pip[0]);
    else
        close(pip[1]);

    dup2(pip[fd], fd);
    close(pip[fd]);
}

int run_process(char * args[], int input_pipe[2], int output_pipe[2]) {
    /* Runs and waits for the process to complete */
    int status;
    int exit_status = 1;

    // Fork a child process
    pid_t pid = fork();

    // TODO for pipe support, you will need to FORK TWICE - once for the first
    // thing, and then once again for the second thing. This gets into some
    // interesting things you could do with streaming?
    /* int _pipe[2]; */
    /* pipe(_pipe); */
    // In parent, print off the process id of child
#ifdef DEBUG
    if (pid > 0) printf("[INFO] Process %d started\n", pid);
#endif

    // Handle fork
    if (pid < 0) {
        // Fork error detected in parent
        fprintf(stderr, "[ERROR] Error forking child process: %s\n", strerror(errno));
    } else if (pid == 0) {
        // In child process...
        /* close(_pipe[1]); */
        // Swaps the input for the child with the pipe output?
        /* swap_fds(_pipe[0], 0); */
        /* handle_file_redirection(inputFile, outputFile); */
        // Child shoudl write to pipe[1]
        // STDOUT for child is now the output from the pipe, close the other end
        // of the pipe
        if (output_pipe != NULL)
            use_pipe(1, output_pipe);
        // STDIN for this is now the input from the pipe, close the other end of
        // the pipe
        if (input_pipe != NULL)
            use_pipe(0, input_pipe);

        if (execvp(args[0], args) < 0) {
            // Error in execvp
            fprintf(stderr, "[ERROR] Error in executing program %s: %s\n", args[0], strerror(errno));
            // Child exit failure
            exit(EXIT_FAILURE);
        }
    } else {
        // Wait in parent process for this pid to finish
        while(waitpid(pid, &status, 0) != pid);

        // Handle different exit / signal cases
        exit_status = handle_process_exit(pid, status);
    }
    return exit_status;
}

void wait_for_process() {
    /* Waits for a child process to finish */
    int status;
    pid_t pid = wait(&status);

    // If there was an error or no child processes are left
    if (pid < 0) {
        if (errno == ECHILD) printf("[INFO] No processes left\n");
        else fprintf(stderr, "[ERROR] Error in wait: %s\n", strerror(errno));
        return;
    }

    // Handle different exit / signal cases
    handle_process_exit(pid, status);
}

int command_dispatcher(char * words[100], int command, int word_n, char * inputFile, char * outputFile) {
    /* Calls different functions based on the command given by user */
    // If the command is not wait (the only non - argument requiring call)
    // And if there are too few args, exit early
    /* if (command != 5 && word_n <= 1) { */
    /*     fprintf(stderr, "[ERROR] Please provide arguments\n"); */
    /*     return 0; */
    /* } */
// TODO Deprecate this
    switch (command) {
        case 0: // Start
            start_process(words + sizeof(char), inputFile, outputFile); // Ignore the first arg: "start"
            break;
        case 1: // Wait
            wait_for_process();
            break;
        case 2: // Kill
        case 3: // Stop
        case 4: { // Continue
            int pid = atoi(words[1]);
            if (pid == 0) {
                fprintf(stderr, "[ERROR] Invalid numeric PID: %s\n", words[1]);
            } else {
                handle_signals(command, pid);
            }
            break;
        }
        case 5: // Exit
            return 1;
        case 6: // Run
            /* run_process(words, inputFile, outputFile); // Ignore the first arg: "run" */
            break;
    }
    return 0;
}

void close_pipe(int in, int out, int * pip) {
    if (in == 1) close(pip[0]);
    if (out == 1) close(pip[1]);
}

int main(int argc, char * argv[]) {
    /* My Shell! */
    int RETURN_STATUS = EXIT_FAILURE;
    char buffer[4096];

    // Show the initial prompt
    prompt(KGRN);

    // Main I/O loop
    while ((fgets(buffer, 4096, stdin) != NULL)) {
        // Initialize vars
        int word_n = 0;

        char * inputFile = NULL;
        char * outputFile = NULL;

        // TODO make an arbitray number of possible things you can fork?
        char ** commands[100];
        char * tempWord;
        char ** command = (char ** ) calloc(sizeof(char), 100);

        int command_n = 0;
        int found_command = 0;

        // Read in from the buffer
        tempWord = strtok(buffer, " \t\n");
        while (tempWord != NULL) {
            found_command = 1;
            // See if we are going to do file redirection
            if (strcmp("<", tempWord) == 0) {
                inputFile = strtok(0, " \t\n");
            } else if (strcmp(">", tempWord) == 0) {
                outputFile = strtok(0, " \t\n");
            } else {
                // If the redirection instruction is attached to the file itself
                if (tempWord[0] == '<')
                    inputFile = tempWord + sizeof(char);
                else if (tempWord[0] == '>')
                    outputFile = tempWord + sizeof(char);
                else if (tempWord[0] == '|') {
                    command[word_n] = 0;
                    commands[command_n++] = command;
                    word_n = 0;
                    command = (char ** ) malloc(sizeof(char) * 100);
                } else {
                    command[word_n++] = tempWord;
                }
            }
            // Get next word
            tempWord = strtok(0, " \t\n");
        }
        // Pads null to the end of the array, adds the latest command to the
        // list of commands
        command[word_n] = 0;
        commands[command_n++] = command;
        // If for some reason we have no commands?
        if (found_command == 0) {
            // Redraw the prompt and try again!
            prompt(KGRN);
            continue;
        }

        // Runs the commands? TODO make it work for arbitray number of procs
        int process_status = 1;
        if (command_n > 1) {
            // Multiple commands
            int i = 0;
            int input_pipe[2];
            pipe(input_pipe);
            // Runs the initial command reading from stdin
            process_status = run_process(commands[i++], NULL, input_pipe);
            // Closes the output portion of the pipe
            close_pipe(0, 1, input_pipe);
            command_n --;

            while (command_n -- > 1) {
                // Run the inner commands that use both pipes
                int new_input_pipe[2];
                pipe(new_input_pipe);
                // Uses the new pipe's stdout as stdout, uses the old pipe's
                // stdin as its stdin
                run_process(commands[i++], input_pipe, new_input_pipe);
                close_pipe(0, 1, new_input_pipe); // Close the input side of the output pipe
                close_pipe(1, 0, input_pipe);
                input_pipe[0] = new_input_pipe[0];
                input_pipe[1] = new_input_pipe[1];
            }

            // Run the last command sending to stdout
            process_status = run_process(commands[i], input_pipe, NULL);
            close_pipe(1, 0, input_pipe); // Close the input part of the output pipe
        } else {
            // Single command
            process_status = run_process(commands[0], NULL, NULL);
        }
        // Redraw the prompt
        char * COLOR;
        switch (process_status) {
            case 0:
                COLOR = KGRN;
                break;
            case 1:
                COLOR = KRED;
                break;
            default:
                COLOR = KRED;
        }
        prompt(COLOR);
    }

    RETURN_STATUS = EXIT_SUCCESS;
    return RETURN_STATUS;
}
