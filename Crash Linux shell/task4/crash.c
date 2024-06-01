
#ifndef MY_HEADER_H
#define MY_HEADER_H
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/wait.h>
#include <errno.h>
#include <bits/waitflags.h>
#include <sys/wait.h>
#include <signal.h>
#endif 



#define SIGKILL 9

#define SIGCHLD 17

#define MAXLINE 1024

#define MAX_JOBS 32  


typedef struct {
    int jobId;     // Unique job identifier
    pid_t pid;     // Process ID associated with this job
    char *cmd;     // Command used to start this job
    char *status;  // "Running", "Stopped", etc.
    bool isKilled; // Whether or not the job is killed
    bool isForeground;  // True if the job is a foreground job
    bool isNotFinished;

} Job;



pid_t fg_pid = 0;
bool fg_isRunning = false;



Job jobs[MAX_JOBS];
int jobCount = 0;  // Number of active jobs
int nextJobId = 1; // Next job ID to be assigned


void sigint_handler(int sig) {
    if (fg_pid > 0) {
        kill(fg_pid, SIGINT);
        for (int i = 0; i < jobCount; i++) {
            if (jobs[i].pid == fg_pid) {
                char jobnum_str[32], pid_str[32];
                snprintf(jobnum_str, sizeof(jobnum_str), "%d", jobs[i].jobId);
                snprintf(pid_str, sizeof(pid_str), "%d", jobs[i].pid);

                write(STDOUT_FILENO, "[", strlen("["));
                write(STDOUT_FILENO, jobnum_str, strlen(jobnum_str));
                write(STDOUT_FILENO, "] (", strlen("] ("));
                write(STDOUT_FILENO, pid_str, strlen(pid_str));
                write(STDOUT_FILENO, ")  killed  ", strlen(")  killed  "));
                write(STDOUT_FILENO, jobs[i].cmd, strlen(jobs[i].cmd));
                write(STDOUT_FILENO, "\n", strlen("\n"));
                jobs[i].isNotFinished = false;

                fg_pid = 0;
                fg_isRunning = false;
                jobs[i].isKilled = true;
                jobs[i].isForeground = false;

                break;
            }
        }
    }
}

void sigquit_handler(int sig) {
    if (fg_pid > 0) {
        kill(fg_pid, SIGQUIT);
        for (int i = 0; i < jobCount; i++) {
            if (jobs[i].pid == fg_pid) {
                char jobnum_str[32], pid_str[32];
                snprintf(jobnum_str, sizeof(jobnum_str), "%d", jobs[i].jobId);
                snprintf(pid_str, sizeof(pid_str), "%d", jobs[i].pid);

                write(STDOUT_FILENO, "[", strlen("["));
                write(STDOUT_FILENO, jobnum_str, strlen(jobnum_str));
                write(STDOUT_FILENO, "] (", strlen("] ("));
                write(STDOUT_FILENO, pid_str, strlen(pid_str));
                write(STDOUT_FILENO, ")  killed (core dumped)  ", strlen(")  killed (core dumped)  "));
                write(STDOUT_FILENO, jobs[i].cmd, strlen(jobs[i].cmd));
                write(STDOUT_FILENO, "\n", strlen("\n"));
                jobs[i].isNotFinished = false;

                fg_pid = 0;
                fg_isRunning = false;
                jobs[i].isKilled = true;
                jobs[i].isForeground = false;

                break;
            }
        }
    } else {
        write(STDOUT_FILENO, "Exiting...\n", strlen("Exiting...\n"));
        exit(0);
    }
}


void sigchld_handler(int sig) {
    pid_t pid;
    int status;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < jobCount; i++) {
            if (jobs[i].pid == pid) {
                char jobnum_str[32], pid_str[32];
                snprintf(jobnum_str, sizeof(jobnum_str), "%d", jobs[i].jobId);
                snprintf(pid_str, sizeof(pid_str), "%d", pid);

                // The child process terminated normally
                if (WIFEXITED(status)) {
                    write(STDOUT_FILENO, "[", strlen("["));
                    write(STDOUT_FILENO, jobnum_str, strlen(jobnum_str));
                    write(STDOUT_FILENO, "] (", strlen("] ("));
                    write(STDOUT_FILENO, pid_str, strlen(pid_str));
                    write(STDOUT_FILENO, ")  finished  ", strlen(")  finished  "));
                    write(STDOUT_FILENO, jobs[i].cmd, strlen(jobs[i].cmd));
                    write(STDOUT_FILENO, "\n", strlen("\n"));
                    jobs[i].isKilled = true;

                }
                // The child process was terminated by a signal
                else if (WIFSIGNALED(status)) {
                    write(STDOUT_FILENO, "[", strlen("["));
                    write(STDOUT_FILENO, jobnum_str, strlen(jobnum_str));
                    write(STDOUT_FILENO, "] (", strlen("] ("));
                    write(STDOUT_FILENO, pid_str, strlen(pid_str));
                    write(STDOUT_FILENO, ")  killed  ", strlen(")  killed  "));
                    if (WCOREDUMP(status)) {
                        write(STDOUT_FILENO, "(core dumped)  ", strlen("(core dumped)  "));
                    }
                    write(STDOUT_FILENO, jobs[i].cmd, strlen(jobs[i].cmd));
                    write(STDOUT_FILENO, "\n", strlen("\n"));
                    jobs[i].isKilled = true;

                }
                // The child process was paused
                else if (WIFSTOPPED(status)) {
                    write(STDOUT_FILENO, "[", strlen("["));
                    write(STDOUT_FILENO, jobnum_str, strlen(jobnum_str));
                    write(STDOUT_FILENO, "] (", strlen("] ("));
                    write(STDOUT_FILENO, pid_str, strlen(pid_str));
                    write(STDOUT_FILENO, ")  suspended  ", strlen(")  suspended  "));
                    write(STDOUT_FILENO, jobs[i].cmd, strlen(jobs[i].cmd));
                    write(STDOUT_FILENO, "\n", strlen("\n"));
                }
                // The child process was resumed
                else if (WIFCONTINUED(status)) {
                    write(STDOUT_FILENO, "[", strlen("["));
                    write(STDOUT_FILENO, jobnum_str, strlen(jobnum_str));
                    write(STDOUT_FILENO, "] (", strlen("] ("));
                    write(STDOUT_FILENO, pid_str, strlen(pid_str));
                    write(STDOUT_FILENO, ")  continued  ", strlen(")  continued  "));
                    write(STDOUT_FILENO, jobs[i].cmd, strlen(jobs[i].cmd));
                    write(STDOUT_FILENO, "\n", strlen("\n"));
                }

                

                break;  // Exit the loop after handling the terminated job
            }
        }
    }
}




void eval(const char **toks, bool bg) {
    if (toks[0] == NULL) return;  // Ignore empty commands
    

    if (strcmp(toks[0], "fg") == 0 && !fg_isRunning) {
    // Ensure exactly one argument is provided
    if (toks[1] == NULL || toks[2] != NULL) {
        fprintf(stderr, "ERROR: fg needs exactly one argument\n");
        return;
    }

    bool isJobId = toks[1][0] == '%';
    bool jobFound = false;

    if (isJobId) {
        // Extract job ID, checking for valid integer
        char* endptr;
        long fgJobId = strtol(&toks[1][1], &endptr, 10);
        if (*endptr != '\0') {
            fprintf(stderr, "ERROR: bad argument for fg: %s\n", toks[1]);
            return;
        }

        // Search for the job by job ID
        for (int i = 0; i < jobCount; i++) {
            if (jobs[i].jobId == fgJobId) {
                jobFound = true;
                int status;
                fg_pid = jobs[i].pid;
                fg_isRunning = true;
                jobs[i].isForeground = true;

                waitpid(jobs[i].pid, &status, WUNTRACED);
                if (jobs[i].isNotFinished){
                    printf("[%d] (%d)  finished  %s\n", jobs[i].jobId, jobs[i].pid, jobs[i].cmd );
                }
                jobs[i].isForeground = false;

                fg_isRunning = false;
                fg_pid = 0;

                jobs[i].isKilled = true;
                break;
            }
        }

        if (!jobFound) {
            fprintf(stderr, "ERROR: no job %%%ld\n", fgJobId);
        }

    } else {
        // Extract process ID, checking for valid integer
        char* endptr;
        long fgPid = strtol(toks[1], &endptr, 10);
        if (*endptr != '\0') {
            fprintf(stderr, "ERROR: bad argument for fg: %s\n", toks[1]);
            return;
        }

        // Search for the job by PID
        for (int i = 0; i < jobCount; i++) {
            if (jobs[i].pid == fgPid) {
                jobFound = true;
                int status;
                fg_pid = jobs[i].pid;
                fg_isRunning = true;
                jobs[i].isForeground = true;


                waitpid(jobs[i].pid, &status, WUNTRACED);
                if (jobs[i].isNotFinished){
                    printf("[%d] (%d)  finished  %s\n", jobs[i].jobId, jobs[i].pid, jobs[i].cmd );
                }
                jobs[i].isForeground = false;

                fg_isRunning = false;
                fg_pid = 0;

                jobs[i].isKilled = true;
                break;
            }
        }

        if (!jobFound) {
            fprintf(stderr, "ERROR: no PID %ld\n", fgPid);
        }
    }


    } else if (strcmp(toks[0], "jobs") == 0) {
        if (toks[1] != NULL) {
            const char *msg = "ERROR: jobs takes no arguments\n";
            write(STDERR_FILENO, msg, strlen(msg));
            return;  
        }
        for (int i = 0; i < jobCount; i++) {
            if (!jobs[i].isKilled){
                printf("[%d] (%d)  %s  %s\n", jobs[i].jobId, jobs[i].pid, jobs[i].status, jobs[i].cmd);
            }
        }
        return;  
    } else if (strcmp(toks[0], "nuke") == 0) {
    if (toks[1] == NULL) {  // No arguments, kill all jobs
        for (int i = 0; i < jobCount; i++) {
            jobs[i].isKilled = true;
            kill(jobs[i].pid, SIGKILL);
        }
    } else {  // Iterate through arguments and kill specified jobs or processes
        for (int i = 1; toks[i] != NULL; i++) {
            if (toks[i][0] == '%') {  // Job ID specified
                char* endptr;
                long jobId = strtol(&toks[i][1], &endptr, 10);

                if (*endptr != '\0' || endptr == &toks[i][1]) {
                    printf("ERROR: bad argument for nuke: %s\n", toks[i]);
                    continue;  // Skip to the next argument
                }

                bool jobFound = false;
                for (int j = 0; j < jobCount; j++) {
                    if (jobs[j].jobId == jobId) {
                        jobs[j].isKilled = true;
                        kill(jobs[j].pid, SIGKILL);
                        jobFound = true;
                        break;
                    }
                }
                if (!jobFound) {
                    printf("ERROR: no job %ld\n", jobId);
                }
            } else {  // Process ID specified
                char* endptr;
                pid_t pid = (pid_t)strtol(toks[i], &endptr, 10);

                if (*endptr != '\0') {
                    printf("ERROR: bad argument for nuke: %s\n", toks[i]);
                    continue;  // Skip to the next argument
                }

                bool jobFound = false;
                for (int j = 0; j < jobCount; j++) {
                    if (jobs[j].pid == pid) {
                        jobs[j].isKilled = true;
                        kill(pid, SIGKILL);
                        jobFound = true;
                        break;
                    }
                }
                if (!jobFound) {
                    printf("ERROR: no PID %d\n", pid);
                }
            }
        }
    }
    return;
    } else if (strcmp(toks[0], "quit") == 0) {
        if (toks[1] != NULL) {
            const char *msg = "ERROR: quit takes no arguments\n";
            write(STDERR_FILENO, msg, strlen(msg));
        } else {
            exit(0);
        }
    } else if (bg){ //background jobs
        if (jobCount >= 32){
            printf("ERROR: too many jobs\n");
            return;
        }
        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGCHLD);
        sigprocmask(SIG_BLOCK, &mask, NULL);
        
        pid_t pid;
        int status;
        pid = fork();

        
        if (pid == 0) {  // Child process
            sigprocmask(SIG_UNBLOCK, &mask, NULL);

            setpgid(0, 0);
            if (execvp(toks[0], (char * const *)toks) < 0) {
                fprintf(stderr, "ERROR: cannot run %s\n", toks[0]);
                exit(1);
            }
        } else {  // Parent process
            sigprocmask(SIG_UNBLOCK, &mask, NULL);

            jobs[jobCount].jobId = nextJobId++;
            jobs[jobCount].pid = pid;
            jobs[jobCount].cmd = strdup(toks[0]);  // Duplicate command string
            jobs[jobCount].status = strdup("running");
            jobs[jobCount].isKilled = false;
            jobs[jobCount].isForeground = false;
            jobs[jobCount].isNotFinished = true;


            jobCount++;
            printf("[%d] (%d)  running  %s\n", jobs[jobCount - 1].jobId, pid , jobs[jobCount-1].cmd);
        }
    } else if (!bg) {  // Foreground job
        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGCHLD);
        sigprocmask(SIG_BLOCK, &mask, NULL);
        pid_t pid = fork();
        
        if (pid == 0) {  // Child process
            sigprocmask(SIG_UNBLOCK, &mask, NULL);

            if (execvp(toks[0], (char * const *)toks) < 0) {
                fprintf(stderr, "ERROR: cannot run %s\n", toks[0]);
                exit(1);
            }
        } else {  // Parent process (shell)
            
            sigprocmask(SIG_UNBLOCK, &mask, NULL);

            int status;
            jobs[jobCount].jobId = nextJobId++;
            jobs[jobCount].pid = pid;
            jobs[jobCount].cmd = strdup(toks[0]);
            jobs[jobCount].isKilled = false;
            jobs[jobCount].isForeground = true;
            jobs[jobCount].isNotFinished = true;

            jobCount++;
            fg_pid = pid;

            fg_isRunning = true;
            

            waitpid(pid, &status, WUNTRACED);

              // Wait for the foreground job to complete

            // After job completion, mark it as no longer in the foreground and remove it from the job list
            for (int i = 0; i < jobCount; i++) {
                if (jobs[i].pid == pid) {
                    fg_pid = 0;
                    fg_isRunning = false;
                    if (jobs[i].isNotFinished){
                        printf("[%d] (%d)  finished  %s\n", jobs[jobCount - 1].jobId, pid, jobs[jobCount - 1].cmd );
                    }
                    jobs[i].isKilled = true;
                    jobs[i].isForeground = false;

                    break;
                }
            }
        }
    }
}



void parse_and_eval(char *s) {
    assert(s);
    const char *toks[MAXLINE+1];
    
    while (*s != '\0') {
        bool end = false;
        bool bg = false;
        int t = 0;

        while (*s != '\0' && !end) {
            while (*s == '\n' || *s == '\t' || *s == ' ') ++s;
            if (*s != ';' && *s != '&' && *s != '\0') toks[t++] = s;
            while (strchr("&;\n\t ", *s) == NULL) ++s;
            switch (*s) {
            case '&':
                bg = true;
                end = true;
                break;
            case ';':
                end = true;
                break;
            }
            if (*s) *s++ = '\0';
        }
        toks[t] = NULL;
        eval(toks, bg);
    }
}

void prompt() {
    const char *prompt = "crash> ";
    ssize_t nbytes = write(STDOUT_FILENO, prompt, strlen(prompt));
}


int repl() {
    char *buf = NULL;
    size_t len = 0;
    while (prompt(), getline(&buf, &len, stdin) != -1) {
        parse_and_eval(buf);
    }

    if (buf != NULL) free(buf);
    if (ferror(stdin)) {
        perror("ERROR");
        return 1;
    }
    return 0;
}

int main(int argc, char **argv) {

    signal(SIGCHLD, sigchld_handler);
    signal(SIGINT, sigint_handler);
    signal(SIGQUIT, sigquit_handler);


    return repl();
}