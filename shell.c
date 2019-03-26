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

void prompt() {
  /* Prints the prompt */
  printf("myshell> ");
  fflush(stdout);
}

int getCommand(char * string) {
  /* Returns the code for the command or prints error if none is found */
  int i;
  int n = 6;
  char * commands[6] = { "start", "wait", "run", "kill", "stop", "continue" };
  for (i = 0; i < n; i ++) {
    // Is it the command?
    if (strcmp(commands[i], string) == 0) return i;
  }

  // Bad command
  fprintf(stderr, "[ERROR] Unknown command: %s\n", string);
  return -1;
}

void handleSignals(int command, int pid) {
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

void handleProcessExit(int pid, int status) {
  /* Handles a successful wait call */
  if (WIFEXITED(status)) {
    printf("[INFO] Process %d exited normally with status %d\n", pid, WEXITSTATUS(status));
  } else if (WIFSIGNALED(status)) {
    printf("[INFO] Process %d exited with signal %d\n", pid, WTERMSIG(status));
  } else if (WIFSTOPPED(status)) {
    printf("[INFO] Process %d stopped with signal %d\n", pid, WSTOPSIG(status));
  } else {
    fprintf(stderr, "[ERROR] Error in processing child process %d\n", pid);
  }
}

void swapFds(char * fname, int fd) {
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

void handleFileRedirection(char * inputFile, char * outputFile) {
  // See if we need to redirect stdin
  if (inputFile != NULL) swapFds(inputFile, 0);
  if (outputFile != NULL) swapFds(outputFile, 1);
}

void startProcess(char * args[], char * inputFile, char * outputFile) {
  /* Starts a child process */
  // Fork a child process
  pid_t pid = fork();

  // In parent, print off the process id of child
  if (pid > 0) printf("[INFO] Process %d started\n", pid);

  // Handle fork
  if (pid < 0) {
    // Fork error detected in parent
    fprintf(stderr, "[ERROR] Error forking child process: %s\n", strerror(errno));
  } else if (pid == 0) {
    // In child process...
    handleFileRedirection(inputFile, outputFile);

    if (execvp(args[0], args) < 0) {
      // Error in execvp
      fprintf(stderr, "[ERROR] Error in executing program %s: %s\n", args[0], strerror(errno));
      // Child exit with failure
      exit(EXIT_FAILURE);
    }
  }
}

void runProcess(char * args[], char * inputFile, char * outputFile) {
  /* Runs and waits for the process to complete */
  int status;

  // Fork a child process
  pid_t pid = fork();

  // In parent, print off the process id of child
  if (pid > 0) printf("[INFO] Process %d started\n", pid);

  // Handle fork
  if (pid < 0) {
    // Fork error detected in parent
    fprintf(stderr, "[ERROR] Error forking child process: %s\n", strerror(errno));
  } else if (pid == 0) {
    // In child process...
    handleFileRedirection(inputFile, outputFile);

    if (execvp(args[0], args) < 0) {
      // Error in execvp
      fprintf(stderr, "[ERROR] Error in executing program %s: %s\n", args[0], strerror(errno));
      // Child exit failure
      exit(EXIT_FAILURE);
    }
  } else {
    // Wait in parent process for this pid to finish
    while(waitpid(pid, &status, 0) != pid) ;

    // Handle different exit / signal cases
    handleProcessExit(pid, status);
  }
}

void waitForProcess() {
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
  handleProcessExit(pid, status);
}

void commandDispatcher(char * words[100], int command, int word_n, char * inputFile, char * outputFile) {
  /* Calls different functions based on the command given by user */
  // If the command is not wait (the only non - argument requiring call)
  // And if there are too few args, exit early
  if (command != 1 && word_n <= 1) {
    fprintf(stderr, "[ERROR] Please provide arguments\n");
    return;
  }

  switch (command) {
    case 0: // Start
      startProcess(words + sizeof(char), inputFile, outputFile); // Ignore the first arg: "start"
      break;
    case 1: // Wait
      waitForProcess();
      break;
    case 2: // Run
      runProcess(words + sizeof(char), inputFile, outputFile); // Ignore the first arg: "run"
      break;
    case 3: // Kill
    case 4: // Stop
    case 5: { // Continue
      int pid = atoi(words[1]);
      if (pid == 0) {
        fprintf(stderr, "[ERROR] Invalid numeric PID: %s\n", words[1]);
      } else {
        handleSignals(command, pid);
      }
      break;
    }
  }
}

int main(int argc, char * argv[]) {
  /* My Shell! */
  int RETURN_STATUS = EXIT_FAILURE;
  char buffer[4096];

  // Show the initial prompt
  prompt();

  // Main I/O loop
  while ((fgets(buffer, 4096, stdin) != NULL)) {
    // Initialize vars
    int word_n = 0;
    int command = -1;

    char * inputFile = NULL;
    char * outputFile = NULL;

    char * words[100];
    char * tempWord;

    // Read in from the buffer
    tempWord = strtok(buffer, " \t\n");
    while (tempWord != NULL) {
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
        else
          words[word_n++] = tempWord;

      }
      // Get next word
      tempWord = strtok(0, " \t\n");
    }
    words[word_n] = 0;

    // If for some reason we have no commands?
    if (word_n <= 0) {
      // Redraw the prompt and try again!
      prompt();
      continue;
    }

    // Parse the command
    command = getCommand(words[0]);

    // Handle specific commands
    commandDispatcher(words, command, word_n, inputFile, outputFile);

    // Redraw the prompt
    prompt();
  }

  RETURN_STATUS = EXIT_SUCCESS;
  return RETURN_STATUS;
}
