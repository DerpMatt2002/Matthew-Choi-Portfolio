/**
 * @file tsh.c
 * @brief A tiny shell program with job control
 *
 * TODO: Delete this comment and replace it with your own.
 * <The line above is not a sufficient documentation.
 *  You will need to write your program documentation.
 *  Follow the 15-213/18-213/15-513 style guide at
 *  http://www.cs.cmu.edu/~213/codeStyle.html.>
 *
 * @author Your Name <andrewid@andrew.cmu.edu>
 * TODO: Matthew Choi mchoi3@andrew.cmu.edu
 */

#include "csapp.h"
#include "tsh_helper.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/*
 * If DEBUG is defined, enable contracts and printing on dbg_printf.
 */
#ifdef DEBUG
/* When debugging is enabled, these form aliases to useful functions */
#define dbg_printf(...) printf(__VA_ARGS__)
#define dbg_requires(...) assert(__VA_ARGS__)
#define dbg_assert(...) assert(__VA_ARGS__)
#define dbg_ensures(...) assert(__VA_ARGS__)
#else
/* When debugging is disabled, no code gets generated for these */
#define dbg_printf(...)
#define dbg_requires(...)
#define dbg_assert(...)
#define dbg_ensures(...)
#endif

/* Function prototypes */
void eval(const char *cmdline);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);
void sigquit_handler(int sig);
void cleanup(void);

/**
 * @brief <Write main's function header documentation. What does main do?>
 *
 * TODO: Delete this comment and replace it with your own.
 *
 * MAIN FUNCTION: the main initiator of the shell program
 */
int main(int argc, char **argv) {
    int c;
    char cmdline[MAXLINE_TSH]; // Cmdline for fgets
    bool emit_prompt = true;   // Emit prompt (default)

    // Redirect stderr to stdout (so that driver will get all output
    // on the pipe connected to stdout)
    if (dup2(STDOUT_FILENO, STDERR_FILENO) < 0) {
        perror("dup2 error");
        exit(1);
    }

    // Parse the command line
    while ((c = getopt(argc, argv, "hvp")) != EOF) {
        switch (c) {
        case 'h': // Prints help message
            usage();
            break;
        case 'v': // Emits additional diagnostic info
            verbose = true;
            break;
        case 'p': // Disables prompt printing
            emit_prompt = false;
            break;
        default:
            usage();
        }
    }

    // Create environment variable
    if (putenv(strdup("MY_ENV=42")) < 0) {
        perror("putenv error");
        exit(1);
    }

    // Set buffering mode of stdout to line buffering.
    // This prevents lines from being printed in the wrong order.
    if (setvbuf(stdout, NULL, _IOLBF, 0) < 0) {
        perror("setvbuf error");
        exit(1);
    }

    // Initialize the job list
    init_job_list();

    // Register a function to clean up the job list on program termination.
    // The function may not run in the case of abnormal termination (e.g. when
    // using exit or terminating due to a signal handler), so in those cases,
    // we trust that the OS will clean up any remaining resources.
    if (atexit(cleanup) < 0) {
        perror("atexit error");
        exit(1);
    }

    // Install the signal handlers
    Signal(SIGINT, sigint_handler);   // Handles Ctrl-C
    Signal(SIGTSTP, sigtstp_handler); // Handles Ctrl-Z
    Signal(SIGCHLD, sigchld_handler); // Handles terminated or stopped child

    Signal(SIGTTIN, SIG_IGN);
    Signal(SIGTTOU, SIG_IGN);

    Signal(SIGQUIT, sigquit_handler);

    // Execute the shell's read/eval loop
    while (true) {
        if (emit_prompt) {
            printf("%s", prompt);

            // We must flush stdout since we are not printing a full line.
            fflush(stdout);
        }

        if ((fgets(cmdline, MAXLINE_TSH, stdin) == NULL) && ferror(stdin)) {
            perror("fgets error");
            exit(1);
        }

        if (feof(stdin)) {
            // End of file (Ctrl-D)
            printf("\n");
            return 0;
        }

        // Remove any trailing newline
        char *newline = strchr(cmdline, '\n');
        if (newline != NULL) {
            *newline = '\0';
        }

        // Evaluate the command line
        eval(cmdline);
    }

    return -1; // control never reaches here
}

// executes BG/FG command
void bgfg_init(struct cmdline_tokens token, int state) {
    sigset_t prev;
    sigset_t mask;
    sigset_t emptymask;

    sigemptyset(&emptymask);
    sigemptyset(&prev);
    // adds set for CHILD, INT and TSTP
    sigaddset(&mask, SIGCHLD);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTSTP);

    char *st;
    if (state != 0) {
        st = "fg";
    } else {
        st = "bg";
    }

    if (token.argv[1] == NULL) {
        sio_printf("%s command requires PID or %%jobid argument\n", st);
        return;
    }

    pid_t pid;
    jid_t jid;

    sigprocmask(SIG_BLOCK, &mask, &prev);
    if (*token.argv[1] == '%') {
        jid = atoi(token.argv[1] + 1);
        // printf("%d\n",jid);
        if (jid <= 0 || !job_exists(jid)) {
            sio_printf("%%%d: No such job\n", jid);
            sigprocmask(SIG_SETMASK, &prev, NULL);
            return;
        }
        pid = job_get_pid(jid);
    } else {
        pid = atoi(token.argv[1]);
        if (job_from_pid(pid) <= 0) {
            sio_printf("%s: argument must be a PID or %%jobid\n", st);
            sigprocmask(SIG_SETMASK, &prev, NULL);
            return;
        }
    }

    //
    if (token.builtin == BUILTIN_BG) {
        kill(-pid, SIGCONT);
        job_set_state(job_from_pid(pid), BG);
        sio_printf("[%d] (%d) %s\n", job_from_pid(pid), pid,
                   job_get_cmdline(job_from_pid(pid)));
        sigprocmask(SIG_SETMASK, &prev, NULL);

    }
    // if built in = FG
    else if (token.builtin == BUILTIN_FG) {
        sigprocmask(SIG_BLOCK, &mask, &prev);
        // if job state = STOP
        /*
        if(job_get_state(job_from_pid(pid)) == ST)
        {
            kill(-pid,SIGCONT);
        }
        */
        kill(-pid, SIGCONT);
        job_set_state(job_from_pid(pid), FG);
        // printf("right before suspend");
        while (fg_job() != 0) {
            sigsuspend(&emptymask);
            // printf("suspending!\n");
        }
        sigprocmask(SIG_SETMASK, &prev, NULL);
        // what do i do here?
    }
}

// returns 1 if valid built in command, 0 otherwise
int built_commands(struct cmdline_tokens token) {
    sigset_t prev;
    sigset_t mask;

    sigemptyset(&prev);
    // adds set for CHILD, INT and TSTP
    sigaddset(&mask, SIGCHLD);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTSTP);

    // finds command quit in line
    if (token.builtin == BUILTIN_QUIT) {
        exit(0);
    }
    // finds command jobs in line
    else if (token.builtin == BUILTIN_JOBS) {
        // list jobs
        // HOW TO MAKE USE OF OUTPUT_FD?
        sigprocmask(SIG_BLOCK, &mask, &prev);

        list_jobs(STDOUT_FILENO);
        sigprocmask(SIG_SETMASK, &prev, NULL);
        return 1;
    }
    // finds bg
    else if (token.builtin == BUILTIN_BG) {
        // execute bg
        bgfg_init(token, 0);
        return 1;
    } else if (token.builtin == BUILTIN_FG) {
        // execute fg
        bgfg_init(token, 1);
        return 1;
    } else {
        return 0;
    }
}

/**
 * @brief <What does eval do?>
 *
 * TODO: Delete this comment and replace it with your own.
 *
 * EVALUATES THE COMMAND LINE WHERE the user has typed in
 *
 * NOTE: The shell is supposed to be a long-running process, so this function
 *       (and its helpers) should avoid exiting on error.  This is not to say
 *       they shouldn't detect and print (or otherwise handle) errors!
 */
void eval(const char *cmdline) {
    parseline_return parse_result;
    struct cmdline_tokens token;
    // keep track of process id
    pid_t pid = 0;
    sigset_t mask;
    sigset_t prev;
    sigset_t emptymask;
    // sigset_t maskall;
    sigemptyset(&emptymask);

    // Parse command line
    parse_result = parseline(cmdline, &token);

    if (parse_result == PARSELINE_ERROR || parse_result == PARSELINE_EMPTY) {
        return;
    }

    // TODO: Implement commands here.

    // if builtin command = quit, quit
    if (!built_commands(token)) {
        // sigfillset(&maskall);
        sigemptyset(&prev);
        // adds set for CHILD, INT and TSTP
        sigaddset(&mask, SIGCHLD);
        sigaddset(&mask, SIGINT);
        sigaddset(&mask, SIGTSTP);

        // temporarily block and save prev block set/BLOCK SIGCHLD
        sigprocmask(SIG_BLOCK, &mask, &prev);
        // child sleeps until receiving
        if ((pid = fork()) == 0) // CHILD RUNS JOB
        {
            setpgid(0, 0);
            // restore previously blocked set
            sigprocmask(SIG_SETMASK, &prev, NULL); // UNBLOCK SIGCHILD
            sigprocmask(SIG_SETMASK, &emptymask, NULL);

            // I/O redirection
            if (token.infile != NULL) {
                int in = open(token.infile, (O_WRONLY | O_RDONLY | O_CREAT), 0);
                dup2(in, STDOUT_FILENO);
                close(in);
            }
            if (token.infile != NULL) {
                int out =
                    open(token.outfile, (O_RDONLY | O_WRONLY | O_CREAT), 0);
                dup2(out, 0);
                close(out);
            }

            if (execve(token.argv[0], token.argv, environ) < 0) {
                sio_printf("%s: No such file or directory\n", token.argv[0]);
                exit(0);
            }

            // less then = input redirection

            // open the file here?
            // open

            // input functionality = infile

            // output = outfile

            // dup2
        }
        // parent process
        // sigprocmask(SIG_BLOCK,&maskall,NULL);
        // sigprocmask(SIG_BLOCK,&mask,&prev);
        // wait for foreground to terminate

        // this part doesnt impact 0-8
        // trace 24 occurs HERE!
        // SUSPEND IS NOT WORKING PROPERLY!
        if (parse_result == PARSELINE_FG) {
            add_job(pid, FG, cmdline);

            /*while FG exists within job list, suspend previous jobs*/
            while (fg_job() != 0) {
                // wait to receive SIGCHLD
                /*
                SIGSUSPEND = same as

                sigprocmask(SIG_BLOCK,mask,prev)
                pause
                sigprocmask(SIG_BLOCK,prev,null)
                */
                // printf("suspend\n");
                sigsuspend(&emptymask);
                // printf("suspending\n");
            }
            // if fg job has finished
            // printf("suspend done\n");
            sigprocmask(SIG_SETMASK, &prev, NULL);
        } else if (parse_result == PARSELINE_BG) {
            // terminate background
            add_job(pid, BG, cmdline);
            sio_printf("[%d] (%d) %s\n", job_from_pid(pid), pid, cmdline);
            sigprocmask(SIG_SETMASK, &prev, NULL);
        }
    }
    return;
}

/*****************
 * Signal handlers
 *****************/

/**
 * @brief <What does sigchld_handler do?>
 *
 * when a child job terminates/stops after receiving SIGSTOP, send
 * signal to sigchld(reaps all zombies, doesnt wait for termination)
 *
 */
void sigchld_handler(int sig) {
    int olderrno = errno;
    pid_t pid;
    int status;
    sigset_t mask;
    sigset_t prev;

    sigemptyset(&prev);
    // REMOVING SIGADDET ON CHLD makes t11 fail?
    sigaddset(&mask, SIGCHLD);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTSTP);
    sigprocmask(SIG_BLOCK, &mask, &prev);

    while ((pid = waitpid(-1, &status, (WNOHANG | WUNTRACED))) > 0)
    // REAPS ALL ZOMBIE CHILDREN
    {
        // block SIGCHLD
        // checks child status
        if (WIFEXITED(status)) {
            // printf("process exit\n");
            delete_job(job_from_pid(pid));
        } else if (WIFSIGNALED(status)) {
            printf("Job [%d] (%d) terminated by signal %d\n", job_from_pid(pid),
                   pid, WTERMSIG(status));
            delete_job(job_from_pid(pid));
        } else if (WIFSTOPPED(status)) {
            sio_printf("Job [%d] (%d) stopped by signal %d\n",
                       job_from_pid(pid), pid, WSTOPSIG(status));
            // set job to stop
            job_set_state(job_from_pid(pid), ST);
        }
        // unblock SIGCHLD
    }
    sigprocmask(SIG_SETMASK, &mask, NULL);
    errno = olderrno;
    return;
}

/**
 * @brief <What does sigint_handler do?>
 *
 * Send a copy whenever the user types ctrl c
 * retrieve PID of process
 * send sigint to the entire process
 */
void sigint_handler(int sig) {
    int olderrno = errno;
    sigset_t mask;
    sigset_t prev;
    sigemptyset(&prev);
    sigaddset(&mask, SIGCHLD);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTSTP);

    // BLOCK SIG_INT
    sigprocmask(SIG_BLOCK, &mask, &prev);

    jid_t jid = fg_job();
    // if jid == 0, unblock and return
    if (jid == 0) {
        sigprocmask(SIG_SETMASK, &prev, NULL);
        errno = olderrno;
        return;
    }

    pid_t pid = job_get_pid(jid);
    // send a sigint to
    // process group id:

    if (kill(-pid, sig) < 0) {
        sio_printf("error when killing sigint");
        return;
    }
    // UNBLOCK SIG_INT
    sigprocmask(SIG_SETMASK, &prev, NULL);
    errno = olderrno;
}

/**
 * @brief <What does sigtstp_handler do?>
 *
 * Send a SIGTSTP to shell when ctrl z
 */
void sigtstp_handler(int sig) {
    int olderrno = errno;
    sigset_t mask;
    sigset_t prev;
    sigemptyset(&prev);
    sigaddset(&mask, SIGCHLD);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTSTP);

    sigprocmask(SIG_BLOCK, &mask, &prev);

    jid_t jid = fg_job();
    if (jid == 0) {
        sigprocmask(SIG_SETMASK, &prev, NULL);
        errno = olderrno;
        return;
    }
    pid_t pid = job_get_pid(jid);
    // send a sigint to
    // process group id:

    // BLOCK SIG_TSTP
    if (kill(-pid, sig) < 0) {
        sio_printf("error when killing sigtstp");
        return;
    }
    // UNBLOCK SIG_TSTP
    sigprocmask(SIG_SETMASK, &prev, NULL);
    errno = olderrno;
}

/**
 * @brief Attempt to clean up global resources when the program exits.
 *
 * In particular, the job list must be freed at this time, since it may
 * contain leftover buffers from existing or even deleted jobs.
 */
void cleanup(void) {
    // Signals handlers need to be removed before destroying the joblist
    Signal(SIGINT, SIG_DFL);  // Handles Ctrl-C
    Signal(SIGTSTP, SIG_DFL); // Handles Ctrl-Z
    Signal(SIGCHLD, SIG_DFL); // Handles terminated or stopped child

    destroy_job_list();
}
