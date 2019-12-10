#include "job.h"

static volatile sig_atomic_t fg_id;

int is_number_str(char *s)
{
    while(*s) {
        if (!isdigit(*s)) {
            return 0;
        }
        s++;
    }
    return 1;
}

int parse_id(char *s)
{
    int error = -1;

    if (s == NULL) {
        return error;
    }
    if (*s == '%') {
        if (is_number_str(s+1))
            return atoi(s+1);
    } else {
        if (is_number_str(s))
            return atoi(s);
    }
    return error;
}

int is_fg_id(pid_t pid)
{
    return fg_id == pid;
}

void set_fg_id(pid_t pid)
{
    fg_id = pid;
}

int get_fg_id()
{
    return fg_id;
}

int add_job_by_pid(pid_t pid, char *cmdline)
{
    int i;

    for (i = 0; i < MAXJOBS; i++) {
        if (joblist[i].used == NO) {
            joblist[i].jid = i;
            joblist[i].pid = pid;
            joblist[i].status = RUNNING;
            joblist[i].used = YES;
            strcpy(joblist[i].cmdline, cmdline);
            return i;
        }
    }
    printf("Not enough job space. MAX %d\n", MAXJOBS);
    return -1;
}

int del_job_by_pid(pid_t pid)
{
    int i;

    for (i = 0; i < MAXJOBS; i++) {
        if (joblist[i].pid == pid) {
            joblist[i].used = NO;
        }
    }
    return 0;
}

void set_job_status(pid_t pid, status_t status)
{
    int i;

    for (i = 0; i < MAXJOBS; i++) {
        if (joblist[i].pid == pid) {
            joblist[i].status = status;
            return;
        }
    }
    printf("Pid %d doesn't exist.\n", pid);
}

JobPtr get_job_by_jid(int jid)
{
    return &joblist[jid];
}

void print_jobs()
{
    int i;

    for (i = 0; i< MAXJOBS; i++) {
        if (joblist[i].used == YES) {
            printf("[%d] %d %s %s\n", joblist[i].jid,
                joblist[i].pid, 
                joblist[i].status? "Running":"Stopped",
                joblist[i].cmdline);
        }
    }
}

void init_jobs()
{
    memset(joblist, 0, sizeof(joblist));
}
