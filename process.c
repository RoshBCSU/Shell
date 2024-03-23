#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s command [args...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();

    if (pid == -1) {
        // If fork() fails, report an error and exit
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process: Attempt to execute the command passed as arguments
        execvp(argv[1], &argv[1]); // argv[1] is the command, &argv[1] is the argument list for the command

        // If execvp returns, it must have failed
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        // Parent process: Wait for the child to finish
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status)) {
            printf("Child exited with status %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Child terminated by signal %d\n", WTERMSIG(status));
        }
    }

    return 0;
}
