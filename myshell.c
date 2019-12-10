#include "shell.h"
#include "job.h"

int main(int argc, char *argv[])
{
    char cmdline[MAXLEN];

    init_jobs();
    if (Signal(SIGTSTP, sigtstp_handler) == SIG_ERR) {
        unix_error("signal stop handler error");
    }
    if (Signal(SIGINT, sigint_handler) == SIG_ERR) {
        unix_error("signal int handler error");
    }
    if (Signal(SIGCHLD, sigchild_handler) == SIG_ERR) {
        unix_error("signal child handler error");
    }
    while (1) {
        printf("> ");
        fgets(cmdline, 100, stdin);
        if (feof(stdin)) {
            exit(0);
        }
        eval(cmdline);
    }
}

void eval(char *cmdline)
{
    char buf[MAXLEN];
    char *argv[MAXARGS];
    int bg;
    pid_t pid;
    int status;
    sigset_t mask_all, mask_one, prev_all, prev_one;
    int jid;

    strcpy(buf, cmdline);
    bg = parse(buf, argv);

    if (argv[0] == NULL) {
        return;
    }

    Sigemptyset(&mask_one);
    Sigaddset(&mask_one, SIGCHLD);

    if (!builtin_cmd(argv)) {
        Sigprocmask(SIG_BLOCK, &mask_one, &prev_one); // ATTENION HERE! block after if;
        if ((pid = Fork()) == 0) {
            Sigprocmask(SIG_SETMASK, &prev_one, NULL);
            Setpgid(0, 0);
            if (execve(argv[0], argv, environ) < 0) {
                printf("%s: unknown command.\n", argv[0]);
                exit(0);
            }
        }
        Sigfillset(&mask_all);
        Sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
        jid = add_job_by_pid(pid, cmdline);
        Sigprocmask(SIG_SETMASK, &prev_all, NULL);

        if (!bg) {
            set_fg_id(pid);
            while (get_fg_id()) {
                Sigsuspend(&prev_one); // Attention here!
            }
        } else {
            printf("[%d] %d %s\n", jid, pid, cmdline);
        }
        Sigprocmask(SIG_SETMASK, &prev_one, NULL);
    } 
    return;
}

void sigchild_handler(int sig)
{
    int status;
    sigset_t mask_all, prev_all;
    pid_t pid;
    int old_errno = errno;

    Sigfillset(&mask_all);
    // Attention below! waitpid, not Waitpid here!
    while ((pid = waitpid(-1, &status, WNOHANG|WUNTRACED|WCONTINUED)) > 0) {
        if (WIFEXITED(status) | WIFSIGNALED(status)) {
            if (is_fg_id(pid)) {
                set_fg_id(0);
            } else {
                sio_puts("process terminated "); sio_putl(pid);
            }
            Sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
            del_job_by_pid(pid);
            Sigprocmask(SIG_SETMASK, &prev_all, NULL);
        } 
        if (WIFSTOPPED(status)) {
            if (is_fg_id(pid)) {
                set_fg_id(0);
            }
            Sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
            set_job_status(pid, STOPPED);
            Sigprocmask(SIG_SETMASK, &prev_all, NULL);
            sio_puts("process stopped"); sio_putl(pid);
        } 
        if (WIFCONTINUED(status)) {
            Sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
            set_job_status(pid, RUNNING);
            printf("process %d continued.\n", pid);
            Sigprocmask(SIG_SETMASK, &prev_all, NULL);
            printf("pid %d continued.\n", pid);
        }
    }
    errno = old_errno;
}

void sigint_handler(int sig)
{
    if (is_fg_id(0)) {
        Signal(SIGINT, SIG_DFL);
        Kill(getpid(), SIGINT);       
    } else {
        Kill(get_fg_id(), SIGINT);       
    }
}

void sigtstp_handler(int sig)
{
    if (is_fg_id(0)) {
        Signal(SIGTSTP, SIG_DFL);
        Kill(getpid(), SIGTSTP);       
    } else {
        Kill(get_fg_id(), SIGTSTP);       
    }
}

int builtin_cmd(char **argv)
{
    pid_t pid;
    int id;
    sigset_t mask_all, prev_all;

    if (!strcmp(argv[0], "quit")) {
        exit(1);
    }
    if (!strcmp(argv[0], "&")) {
        return 1;
    }
    if (!strcmp(argv[0], "jobs")) {
        print_jobs();
        return 1;
    }
    if (!strcmp(argv[0], "fg")) {
        id = parse_id(argv[1]);
        if (id != -1 && argv[2] == NULL) {
            if (*argv[1] == '%') {
                JobPtr jp = get_job_by_jid(id);
                pid = jp->pid;
            }
            else {
                pid = id;
            }
            set_fg_id(pid);
            Sigfillset(&mask_all);
            Sigprocmask(SIG_SETMASK, &mask_all, &prev_all);
            set_job_status(pid, RUNNING);
            Kill(pid, SIGCONT);
            while (get_fg_id()) {
                Sigsuspend(&prev_all);
            }
            // set_fg_id(0);
            Sigprocmask(SIG_SETMASK, &prev_all, NULL);
        } else {
            printf("Usage: fg <%%jobid>|<processid>\n");
        }
        return 1;
    }
    if (!strcmp(argv[0], "bg")) {
        id = parse_id(argv[1]);
        if (id != -1 && (argv[2] == NULL)) {
            if (*argv[1] == '%') {
                JobPtr jp = get_job_by_jid(id);
                pid = jp->pid;
            } else {
                pid = id;
            }
            set_job_status(pid, RUNNING);
            Kill(pid, SIGCONT);
        } else {
            printf("Usage: bf <%%jobid>|<processid>\n");
        }
        return 1;
    }
    return 0;
}

int parse(char *buf, char *argv[])
{
    int argc;
    char *delim;
    int bg;

    buf[strlen(buf) -1] = ' ';
    while (*buf == ' ')
        buf++;
    argc = 0;
    while ((delim = strchr(buf, ' '))) {
        argv[argc++] = buf;
        *delim = '\0';
        buf = delim + 1;
        while (*buf && (*buf == ' ')) {
            buf++;
        }
    }
    argv[argc] = NULL;

    if (argv == 0) {
        return 1;
    }

    if ((bg = (*argv[argc-1] == '&')) != 0) {
        argv[--argc] = NULL;
    }

    return bg;
}

void unix_error(char *msg) 
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}

/************************************
 * Wrappers for Unix signal functions 
 ***********************************/

/* $begin sigaction */
handler_t *Signal(int signum, handler_t *handler) 
{
    struct sigaction action, old_action;

    action.sa_handler = handler;  
    sigemptyset(&action.sa_mask); /* Block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* Restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
	unix_error("Signal error");
    return (old_action.sa_handler);
}
/* $end sigaction */

void Sigprocmask(int how, const sigset_t *set, sigset_t *oldset)
{
    if (sigprocmask(how, set, oldset) < 0)
	unix_error("Sigprocmask error");
    return;
}

void Sigemptyset(sigset_t *set)
{
    if (sigemptyset(set) < 0)
	unix_error("Sigemptyset error");
    return;
}

void Sigfillset(sigset_t *set)
{ 
    if (sigfillset(set) < 0)
	unix_error("Sigfillset error");
    return;
}

void Sigaddset(sigset_t *set, int signum)
{
    if (sigaddset(set, signum) < 0)
	unix_error("Sigaddset error");
    return;
}

void Sigdelset(sigset_t *set, int signum)
{
    if (sigdelset(set, signum) < 0)
	unix_error("Sigdelset error");
    return;
}

int Sigismember(const sigset_t *set, int signum)
{
    int rc;
    if ((rc = sigismember(set, signum)) == 0)
	unix_error("Sigismember error");
    return rc;
}

int Sigsuspend(const sigset_t *set)
{
    int rc = sigsuspend(set); /* always returns -1 */
    if (errno != EINTR)
        unix_error("Sigsuspend error");
    return rc;
}

void Kill(pid_t pid, int signum) 
{
    int rc;

    if ((rc = kill(pid, signum)) < 0)
	unix_error("Kill error");
}


/*********************************************
 * Wrappers for Unix process control functions
 ********************************************/

/* $begin forkwrapper */
pid_t Fork(void) 
{
    pid_t pid;

    if ((pid = fork()) < 0)
	unix_error("Fork error");
    return pid;
}
/* $end forkwrapper */

void Execve(const char *filename, char *const argv[], char *const envp[]) 
{
    if (execve(filename, argv, envp) < 0)
	unix_error("Execve error");
}

/* $begin wait */
pid_t Wait(int *status) 
{
    pid_t pid;

    if ((pid  = wait(status)) < 0)
	unix_error("Wait error");
    return pid;
}
/* $end wait */

pid_t Waitpid(pid_t pid, int *iptr, int options) 
{
    pid_t retpid;

    if ((retpid  = waitpid(pid, iptr, options)) < 0) 
	unix_error("Waitpid error");
    return(retpid);
}

void Pause() 
{
    (void)pause();
    return;
}

unsigned int Sleep(unsigned int secs) 
{
    unsigned int rc;

    if ((rc = sleep(secs)) < 0)
	unix_error("Sleep error");
    return rc;
}

unsigned int Alarm(unsigned int seconds) {
    return alarm(seconds);
}
 
void Setpgid(pid_t pid, pid_t pgid) {
    int rc;

    if ((rc = setpgid(pid, pgid)) < 0)
	unix_error("Setpgid error");
    return;
}

pid_t Getpgrp(void) {
    return getpgrp();
}

/* Public Sio functions */
/* $begin siopublic */

ssize_t sio_puts(char s[]) /* Put string */
{
    return write(STDOUT_FILENO, s, sio_strlen(s)); //line:csapp:siostrlen
}

ssize_t sio_putl(long v) /* Put long */
{
    char s[128];
    
    sio_ltoa(v, s, 10); /* Based on K&R itoa() */  //line:csapp:sioltoa
    return sio_puts(s);
}

void sio_error(char s[]) /* Put error message and exit */
{
    sio_puts(s);
    _exit(1);                                      //line:csapp:sioexit
}
/* $end siopublic */

/*************************************************************
 * The Sio (Signal-safe I/O) package - simple reentrant output
 * functions that are safe for signal handlers.
 *************************************************************/

/* Private sio functions */

/* $begin sioprivate */
/* sio_reverse - Reverse a string (from K&R) */
static void sio_reverse(char s[])
{
    int c, i, j;

    for (i = 0, j = strlen(s)-1; i < j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

/* sio_ltoa - Convert long to base b string (from K&R) */
static void sio_ltoa(long v, char s[], int b) 
{
    int c, i = 0;
    int neg = v < 0;

    if (neg)
	v = -v;

    do {  
        s[i++] = ((c = (v % b)) < 10)  ?  c + '0' : c - 10 + 'a';
    } while ((v /= b) > 0);

    if (neg)
	s[i++] = '-';

    s[i] = '\0';
    sio_reverse(s);
}

/* sio_strlen - Return length of string (from K&R) */
static size_t sio_strlen(char s[])
{
    int i = 0;

    while (s[i] != '\0')
        ++i;
    return i;
}
/* $end sioprivate */

/* Public Sio functions */