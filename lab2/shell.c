#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

#define MAX_LINE 80 /* 80 chars per line, per command */
#define MAX_ARGS (MAX_LINE / 2 + 1) /* command line (of 80) has max of 40 arguments */
#define DELIMITER " \n"

void init_args(char *args[]);
void refresh_args(char *args[]);
void init_command(char *command);
int get_input(char *command);
int parse_input(char *args[], char *original_command);
void run_command(char *args[], int args_num);
void check_pipe(char *args[], int *args_num, char **args2[], int *args_num2);
int io_redirection(char *args[], int *args_num);
void close_redirection(int fd);
void execute_command(char *args[]);

int main(void) {
	char *args[MAX_ARGS];
	char command[MAX_LINE];
	int should_run = 1;

	init_args(args);
	init_command(command);
	while (should_run) {
		printf("osh>");
		fflush(stdout);
		fflush(stdin);

		refresh_args(args);

		if (!get_input(command)) continue;

		int args_num = parse_input(args, command);

        if (args_num == 0) continue;
        if (strcmp(args[0], "exit") == 0) break;

        run_command(args, args_num);
	}

    refresh_args(args);

	return 0;
}

void init_args(char *args[]) {
	int i;
    for(i = 0; i < MAX_ARGS; i++)
        args[i] = NULL;
}

void refresh_args(char *args[]) {
    while(*args) {
        free(*args);
        *args++ = NULL;
    }
}

void init_command(char *command) {
    strcpy(command, "");
}

int get_input(char *command) {
    char input_buffer[MAX_LINE];
    fgets(input_buffer, MAX_LINE, stdin);
	
    if(strncmp(input_buffer, "!!", 2) == 0) {
        if(strlen(command) == 0) {
            fprintf(stderr, "No commands in history.\n");
            return 0;
        }
        printf("%s", command);
        return 1;
    }
    strcpy(command, input_buffer);
    return 1;
}

int parse_input(char *args[], char *original_command) {
    char command[MAX_LINE];
    strcpy(command, original_command);
    char *token = strtok(command, DELIMITER);

    int i;
    for (i = 0; token != NULL; i++) {
        args[i] = malloc(strlen(token) + 1);
        strcpy(args[i], token);
        token = strtok(NULL, DELIMITER);
    }
    return i;
}

void run_command(char *args[], int args_num) {
    char **args2;
    int args_num2;

    args_num2 = 0;
    check_pipe(args, &args_num, &args2, &args_num2);

    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Fork Failed\n");
    } else if (pid == 0) {
        int iofd;

        if (args_num2 == 0) {
            iofd = io_redirection(args, &args_num);
            execute_command(args);
            close_redirection(iofd);
        } else {
            int fd[2];
            pipe(fd);

            pid_t pid2 = fork();
            if (pid < 0) {
                fprintf(stderr, "Fork Failed\n");
            } else if (pid2 == 0) {
                /* First Command */
                iofd = io_redirection(args, &args_num);
                close(fd[0]);
                dup2(fd[1], STDOUT_FILENO);
                execute_command(args);
                close_redirection(iofd);
                close(fd[1]);
            } else {
                /* Second Command */
                iofd = io_redirection(args2, &args_num2);
                close(fd[1]);
                dup2(fd[0], STDIN_FILENO);
                wait(NULL);
                execute_command(args2);
                close_redirection(iofd);
                close(fd[0]);
            }
        }
    } else {
        wait(NULL);
    }
}

void check_pipe(char *args[], int *args_num, char **args2[], int *args_num2) {
    int i;
    for (i = 0; i < *args_num; i++) {
        if (strcmp(args[i], "|") == 0) {
            free(args[i]);
            args[i] = NULL;

            *args2 = args + i + 1;
            *args_num2 = *args_num - i - 1;
            *args_num = i;
            break;
        }
    }
}

int io_redirection(char *args[], int *args_num) {
    int fd = -1;

    int i;
    for (i = 0; i < *args_num; i++){
        if (strcmp("<", args[i]) == 0 && i < *args_num - 1) {
            char *pathname = args[i+1];

            mode_t read_mask = S_IRUSR | S_IRGRP | S_IROTH;

            fd = open(pathname, O_RDONLY, read_mask);
            dup2(fd, STDIN_FILENO);
            
            free(args[i]);
            free(args[i+1]);
            args[i] = NULL;
            args[i+1] = NULL;
            *args_num -= 2;

            break;
        }
        if (strcmp(">", args[i]) == 0 && i < *args_num - 1) {
            char *pathname = args[i+1];

            mode_t read_mask = S_IRUSR | S_IRGRP | S_IROTH;
            mode_t write_mask = S_IWUSR | S_IWGRP | S_IWOTH;

            fd = open(pathname, O_WRONLY | O_CREAT, read_mask | write_mask);
            dup2(fd, STDOUT_FILENO);

            free(args[i]);
            free(args[i+1]);
            args[i] = NULL;
            args[i+1] = NULL;
            *args_num -= 2;

            break;
        }
    }

    return fd;
}

void close_redirection(int fd) {
    if (fd != -1) close(fd);
}

void execute_command(char *args[]) {
    int err = execvp(args[0], args);
    if (err == -1) {
        fprintf(stderr, "Command not found\n");
        exit(errno);
    }
}
