#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

pid_t foreground_pid = 0;
// Environment variable array
extern char **environ;

// Signal handler for SIGINT
void handle_sigint(int sig) {
    // When ctrl-C is pressed, this message is shown
    printf("\nCaught signal %d. Shell does not exit. Type 'exit' to quit shell.\n", sig);
    printf("Shell> ");
    fflush(stdout); // Ensure the new prompt is displayed immediately
}

void handle_sigalrm(int sig) {
    if (foreground_pid != 0) {
        kill(foreground_pid, SIGKILL); // Terminate the foreground process
        printf("\nForeground process %d terminated due to timeout\n", foreground_pid);
        foreground_pid = 0; // Reset the foreground PID
    }
}

// Function prototypes for built-in commands
void cd_command(char **args);
void pwd_command();
void echo_command(char **args);
void env_command();
void setenv_command(char **args);
void exit_shell();

// Execute a command
void execute_command(char *cmd, char **args, int background) {
        if (strcmp(cmd, "cd") == 0) {
        cd_command(args);
    } else if (strcmp(cmd, "pwd") == 0) {
        pwd_command();
    } else if (strcmp(cmd, "echo") == 0) {
        echo_command(args);
    } else if (strcmp(cmd, "env") == 0) {
        env_command();
    } else if (strcmp(cmd, "setenv") == 0) {
        setenv_command(args);
    } else if (strcmp(cmd, "exit") == 0) {
        exit_shell();
    } else {
        pid_t pid = fork();
        if (pid == 0) {
            // Child process: Execute the command
            if (execvp(cmd, args) == -1) {
                perror("execvp");
                exit(EXIT_FAILURE);
            }
        } else if (pid > 0) {
            // Parent process
            if (!background) {
                foreground_pid = pid; // Set the foreground PID for signal handling
                alarm(10); // Set an alarm for 10 seconds to potentially terminate this process
                
                int status;
                waitpid(pid, &status, 0); // Wait for the foreground process to finish
                
                alarm(0); // Cancel any pending alarm if the process finishes before timeout
                foreground_pid = 0; // Reset the foreground PID as the process has completed
            } else {
                printf("Process %d running in background\n", pid);
            }
        } else {
            perror("fork");
        }
    }
}


// Change directory
void cd_command(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "Expected argument to \"cd\"\n");
    } else {
        if (chdir(args[1]) != 0) {
            perror("cd");
        }
    }
}

// Print working directory
void pwd_command() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("pwd");
    }
}

// Echo command
void echo_command(char **args) {
    for (int i = 1; args[i] != NULL; i++) {
        printf("%s ", args[i]);
    }
    printf("\n");
}

// Print environment variables
void env_command() {
    for (char **env = environ; *env != 0; env++) {
        char *thisEnv = *env;
        printf("%s\n", thisEnv);
    }
}

// Set environment variable
void setenv_command(char **args) {
    if (args[1] == NULL || args[2] == NULL) {
        fprintf(stderr, "Expected two arguments for \"setenv\"\n");
    } else {
        if (setenv(args[1], args[2], 1) != 0) {
            perror("setenv");
        }
    }
}

// Exit the shell
void exit_shell() {
    exit(0);
}

int main() {
    // Register SIGINT signal handler
    struct sigaction sa_int, sa_alrm;
    memset(&sa_int, 0, sizeof(sa_int));
    sa_int.sa_handler = handle_sigint;
    sigaction(SIGINT, &sa_int, NULL);

    // Register SIGALRM signal handler
    memset(&sa_alrm, 0, sizeof(sa_alrm));
    sa_alrm.sa_handler = handle_sigalrm;
    sigaction(SIGALRM, &sa_alrm, NULL);

    while(1) {
        printf("Shell> ");
        fflush(stdout);

        char *line = NULL;
        size_t len = 0;
        getline(&line, &len, stdin);

        // Check for "exit" command
        if (strncmp(line, "exit", 4) == 0) {
            free(line);
            break;
        }

        char *args[10]; // Support up to 10 arguments
        int argc = 0, background = 0;
        char *token = strtok(line, " \t\n");
        while (token != NULL) {
            args[argc++] = token;
            token = strtok(NULL, " \t\n");
        }
        args[argc] = NULL;
        // Check for a background process request
        if (argc > 0 && strcmp(args[argc - 1], "&") == 0) {
            background = 1;
            args[--argc] = NULL; // Remove '&' from arguments
        }

        // Execute the command
        execute_command(args[0], args, background);

        free(line); // Free the line read
    }

    return 0;
}
