#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/wait.h>
#include <errno.h>


#define MAXLINE 1024

typedef struct {
    int jobId;     // Unique job identifier
    pid_t pid;     // Process ID associated with this job
    char *cmd;     // Command used to start this job
    char *status;  // "Running", "Stopped", etc.
} Job;

#define MAX_JOBS 32  

Job jobs[MAX_JOBS];
int jobCount = 0;  // Number of active jobs
int nextJobId = 1; // Next job ID to be assigned

void eval(const char **toks, bool bg) {
    if (toks[0] == NULL) return;  // Ignore empty commands
    
    if (strcmp(toks[0], "quit") == 0) {
        if (toks[1] != NULL) {
            const char *msg = "ERROR: quit takes no arguments\n";
            write(STDERR_FILENO, msg, strlen(msg));
        } else {
            exit(0);
        }
    } 
    else if (bg){
        if (jobCount >= 32){
            printf("ERROR too many jobs running in background\n");
            return;
        }
        pid_t pid;
        int status;

        pid = fork();
        if (pid == 0) {  // Child process
            if (execvp(toks[0], (char * const *)toks) < 0) {
                fprintf(stderr, "ERROR: cannot run %s\n", toks[0]);
                exit(1);
            }
        } else {  // Parent process
            jobs[jobCount].jobId = nextJobId++;
            jobs[jobCount].pid = pid;
            jobs[jobCount].cmd = strdup(toks[0]);  // Duplicate command string
            jobs[jobCount].status = strdup("Running");
            jobCount++;
            printf("[%d] (%d)  running  %s\n", jobs[jobCount - 1].jobId, pid, jobs[jobCount-1].cmd);
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
    return repl();
}
