#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
extern char **environ;
#define MAXLEN 100
#define MAXARGS 10
typedef void handler_t(int);

void eval(char *cmdline);
void sigchild_handler(int sig);
void sigint_handler(int sig);
void sigtstp_handler(int sig);
int builtin_cmd(char **argv);
int parse(char *buf, char *argv[]);
void unix_error(char *msg);
handler_t *Signal(int signum, handler_t *handler);
void Sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
void Sigemptyset(sigset_t *set);
void Sigfillset(sigset_t *set);
void Sigaddset(sigset_t *set, int signum);
void Sigdelset(sigset_t *set, int signum);
int Sigismember(const sigset_t *set, int signum);
int Sigsuspend(const sigset_t *set);
void Kill(pid_t pid, int signum);
pid_t Fork(void);
void Execve(const char *filename, char *const argv[], char *const envp[]);
pid_t Wait(int *status);
pid_t Waitpid(pid_t pid, int *iptr, int options);
void Pause();
unsigned int Sleep(unsigned int secs);
unsigned int Alarm(unsigned int seconds);
void Setpgid(pid_t pid, pid_t pgid);
pid_t Getpgrp(void);
ssize_t sio_putl(long v); /* Put long */
ssize_t sio_puts(char s[]); /* Put string */
void sio_error(char s[]); /* Put error message and exit */
static size_t sio_strlen(char s[]);
static void sio_reverse(char s[]);
static void sio_ltoa(long v, char s[], int b);


