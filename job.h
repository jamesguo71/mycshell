#include "shell.h"
#include <ctype.h>
typedef enum {STOPPED, RUNNING} status_t;
#define MAXJOBS 100
#define YES 1
#define NO 0
struct job {
    int jid;
    pid_t pid;
    int used;
    status_t status;
    char cmdline[MAXLEN];
} joblist[MAXJOBS];

typedef struct job *JobPtr;
int add_job_by_pid(pid_t pid, char *cmdline);
int del_job_by_pid(pid_t pid);
int get_fg_id();
JobPtr get_job_by_jid(int jid);
void init_jobs();
int is_fg_id(pid_t pid);
int is_number_str(char *s);
int parse_id(char *s);
void print_jobs();
void set_fg_id(pid_t pid);
void set_job_status(pid_t pid, status_t status);