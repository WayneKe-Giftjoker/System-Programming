#ifndef THREADTOOL
#define THREADTOOL
#include <setjmp.h>
#include <sys/signal.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <poll.h>
#include <fcntl.h>

#define THREAD_MAX 16  // maximum number of threads created
#define BUF_SIZE 512
struct tcb {
    int id;  // the thread id
    jmp_buf environment;  // where the scheduler should jump to
    int arg;  // argument to the function
    int fd;  // file descriptor for the thread
    char buf[BUF_SIZE];  // buffer for the thread
    int i, x, y;  // declare the variables you wish to keep between switches
};

extern int timeslice;
extern jmp_buf sched_buf;
extern jmp_buf MAIN;
extern struct tcb *ready_queue[THREAD_MAX], *waiting_queue[THREAD_MAX];
extern struct pollfd fdarr[THREAD_MAX];
/*
 * rq_size: size of the ready queue
 * rq_current: current thread in the ready queue
 * wq_size: size of the waiting queue
 */
extern int rq_size, rq_current, wq_size;
/*
* base_mask: blocks both SIGTSTP and SIGALRM
* tstp_mask: blocks only SIGTSTP
* alrm_mask: blocks only SIGALRM
*/
extern sigset_t base_mask, tstp_mask, alrm_mask;
/*
 * Use this to access the running thread.
 */
#define RUNNING (ready_queue[rq_current])

void sighandler(int signo);
void scheduler();

#define thread_create(func, id, arg) {                                                              \
    if(setjmp(MAIN) == 0){                                                                          \
        func(id, arg);                                                                              \
    }                                                                                               \
}

#define thread_setup(id, arg) {                                                                     \
    struct tcb* newThrd = (struct tcb*)malloc(sizeof(struct tcb));                                  \
    char filename[1<<4];                                                                            \
    char buf[BUF_SIZE];                                                                             \
    snprintf(filename, 1<<4, "%s", __FUNCTION__);                                                   \
    snprintf(buf, BUF_SIZE, "%d_%s", id, filename);                                                 \
    mkfifo(buf, 0666);                                                                              \
    newThrd->fd = open(buf, O_RDONLY | O_NONBLOCK);                                                 \
    newThrd->id = id;                                                                               \
    newThrd->arg = arg;                                                                             \
    ready_queue[rq_size] = newThrd;                                                                 \
    ++rq_size;                                                                                      \
    if(setjmp(newThrd->environment) == 0){                                                          \
        longjmp(MAIN, -1);                                                                          \
    }                                                                                               \
}

#define thread_exit() {                                                                             \
    char filename[1<<4];                                                                            \
    char buf[BUF_SIZE];                                                                             \
    snprintf(filename, 1<<4, "%s", __FUNCTION__);                                                   \
    snprintf(buf, BUF_SIZE, "%d_%s", RUNNING->id, filename);                                        \
    unlink(buf);                                                                                    \
    close(RUNNING->fd);                                                                             \
    longjmp(sched_buf, 3);                                                                          \
}

#define thread_yield() {                                                                            \
    sigset_t pnd_mask;                                                                              \
    sigemptyset(&pnd_mask);                                                                         \
    sigpending(&pnd_mask);                                                                          \
    int ret_val = setjmp(RUNNING->environment);                                                     \
    if(!ret_val){                                                                                   \
        if(sigismember(&pnd_mask, SIGTSTP)){                                                        \
            sigprocmask(SIG_UNBLOCK, &tstp_mask, NULL);                                             \
        }                                                                                           \
        else if(sigismember(&pnd_mask, SIGALRM)){                                                   \
            sigprocmask(SIG_UNBLOCK, &alrm_mask, NULL);                                             \
        }                                                                                           \
    }                                                                                               \
}

#define async_read(count) ({                                                                        \
    int ret_val = setjmp(RUNNING->environment);                                                     \
    if(ret_val == 0){                                                                               \
        longjmp(sched_buf, 2);                                                                      \
    }                                                                                               \
    else{                                                                                           \
        read(RUNNING->fd, RUNNING->buf, count);                                                     \
    }                                                                                               \
})

#endif // THREADTOOL
