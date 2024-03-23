#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

// Forward declarations
void execute_command(char *cmd, char **args);
void cd_command(char **args);
void pwd_command();
void echo_command(char **args);
void env_command();
void setenv_command(char **args);
void exit_command();
void sigint_handler(int sig) {   
    write(STDOUT_FILENO, "\n> ", 3); 
}

// Environment variable array
extern char **environ;

int main() {
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;
    char *token;
    setenv("OSTYPE", "linux-gnu", 1);
    const char *delim = " \t\n";
    signal(SIGINT, sigint_handler);
    while (1) {
        printf("> ");
        nread = getline(&line, &len, stdin);
        if (nread == -1) break;

        // Tokenize the input
        char *args[10]; // Support for up to 10 arguments
        int argc = 0;
        token = strtok(line, delim);
        while (token != NULL && argc < 9) {
            args[argc++] = token;
            token = strtok(NULL, delim);
        }
        args[argc] = NULL; // Null-terminate the arguments array

        if (argc == 0) continue; // Skip empty inputs

        // Execute built-in commands
        execute_command(args[0], args);
    }

    free(line);
    return 0;
}

void execute_command(char *cmd, char **args) {
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
        exit_command();
    } else {
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            if (execvp(cmd, args) == -1) {
                perror("execvp");
                exit(EXIT_FAILURE); // Ensure child process exits if exec fails
            }
        } else if (pid > 0) {
            // Parent process
            int status;
            do {
                waitpid(pid, &status, WUNTRACED);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        } else {
            // Forking failed
            perror("fork");
        }
    }
}

void cd_command(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "cd: expected argument\n");
    } else {
        if (chdir(args[1]) != 0) {
            perror("cd");
        }
    }
}

void pwd_command() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("pwd");
    }
}

void echo_command(char **args) {
    for (int i = 1; args[i] != NULL; i++) {
        // Directly check if the argument is an environment variable
        if (args[i][0] == '$') {
            // Assume it's an environment variable and attempt to expand it
            char* varName = args[i] + 1; // Skip the '$' to get the variable name
            char* value = getenv(varName); // Attempt to get the variable's value from the environment
            if (value) {
                printf("%s ", value); // If the variable exists, print its value
            } else {
            }
        } else {
            // If the argument isn't an environment variable, print it as is
            printf("%s ", args[i]);
        }
    }
    printf("\n"); // Print a newline at the end of the command output
}

void env_command() {
    for (char **env = environ; *env != 0; env++) {
        char *thisEnv = *env;
        printf("%s\n", thisEnv);
    }
}

void setenv_command(char **args) {
    if (args[1] == NULL || args[2] == NULL) {
        fprintf(stderr, "setenv: expected two arguments\n");
    } else {
        if (setenv(args[1], args[2], 1) != 0) {
            perror("setenv");
        }
    }
}

void exit_command() {
    exit(0);
}
