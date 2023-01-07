//discussed with b09902129 黃柏鈞
#include "threadtools.h"
#include <poll.h>
#include <errno.h>
#include <string.h>

jmp_buf MAIN;
struct pollfd fdarr[THREAD_MAX];
struct pollfd testfdarr[THREAD_MAX];

/*
 * Print out the signal you received.
 * If SIGALRM is received, reset the alarm here.
 * This function should not return. Instead, call longjmp(sched_buf, 1).
 */
void sighandler(int signo) {
    // TODO
    if(signo == SIGTSTP){
        sigprocmask(SIG_SETMASK, &base_mask, NULL);
        printf("caught SIGTSTP\n");
    }
    else if(signo == SIGALRM){
        sigprocmask(SIG_SETMASK, &base_mask, NULL);
        printf("caught SIGALRM\n");
        alarm(timeslice);
    }
    longjmp(sched_buf, 1);
}

/*
 * Prior to calling this function, both SIGTSTP and SIGALRM should be blocked.
 */
void scheduler() {
    // TODO
    int state = setjmp(sched_buf);
    // memset(&testfdarr, 0, sizeof(testfdarr));
    // for(int i = 0; i < wq_size; ++i){
    //     testfdarr[i].fd = waiting_queue[i]->fd;
    //     testfdarr[i].events = POLLIN;
    // }
    if(state == 0){ //from main
        rq_current = 0;
        longjmp(RUNNING->environment, -1);
    }
    else{
        poll(fdarr, wq_size, 0);
        //fprint(stderr, "move finished waiting queue to ready queue\n");
        for(int i = 0; i < wq_size; ++i){
            if(fdarr[i].revents & POLLIN){ //ready 2 read
                // fprintf(stderr, "=id: %d ready to read\n", waiting_queue[i]->id);
                ready_queue[rq_size] = waiting_queue[i];
                ++rq_size;
                waiting_queue[i] = NULL;
            }
        }
        int newWQ_size = 0;
        // if(wq_size > 0)
        //     fprintf(stderr, "renew waiting queue: \n");
        for(int i = 0; i < wq_size; ++i){
            if(waiting_queue[i] == NULL){
                memset(&fdarr+i, 0, sizeof(struct pollfd));
                int j = i+1;
                for(; j < wq_size; ++j){
                    if(waiting_queue[j] != NULL){
                        break;
                    }
                }
                if(j < wq_size){
                    waiting_queue[newWQ_size] = waiting_queue[j];
                    fdarr[newWQ_size] = fdarr[j];
                    memset(&fdarr+j, 0, sizeof(struct pollfd));
                    ++newWQ_size;
                }
                i = j;
            }
            else{
                waiting_queue[newWQ_size] = waiting_queue[i];
                fdarr[newWQ_size] = fdarr[i];
                if(newWQ_size != i){
                    memset(&fdarr+i, 0, sizeof(struct pollfd));
                }
                ++newWQ_size;
            }
        }
        //fprint(stderr, "old_wq_size = %d, new_wq_size = %d\n", wq_size, newWQ_size);
        wq_size = newWQ_size;
        
        if(state == 2 || state == 3){ //Remove the current thread from the ready queue if needed.
            if(state == 2){
                //fprint(stderr, "state 2| async_read\n");
                waiting_queue[wq_size] = RUNNING;
                fdarr[wq_size].fd = RUNNING->fd;
                fdarr[wq_size].events = POLLIN;
                ++wq_size;
            }
            else{
                //fprint(stderr, "state 3| exit\n");
                close(RUNNING->fd);
                // fdarr[RUNNING->id].events = 0;
                // memset((&fdarr + RUNNING->id), 0, sizeof(struct pollfd));
                free(RUNNING);
            }
            if(rq_current != rq_size-1){
                RUNNING = ready_queue[rq_size-1];
            }
            --rq_size;
        }

        //Switch to the next thread.
        //fprint(stderr, "switch to the next thread\n");
        if(rq_size > 0){
            //fprint(stderr, "rq_size > 0\n");
            if(state == 1){
                rq_current = (rq_current+1)%rq_size;
            }
            else if(state == 2 || state == 3){
                if(rq_current == rq_size){
                    rq_current = 0;
                }
            }
        }
        else{
            // fprintf(stderr, "rq_size = %d || wq_size = %d\n", rq_size, wq_size);
            if(wq_size > 0){
                // memset(&testfdarr, 0, sizeof(testfdarr));
                // for(int i = 0; i < wq_size; ++i){
                //     testfdarr[i].fd = waiting_queue[i]->fd;
                //     testfdarr[i].events = POLLIN;
                // }
                
                // for(int i = 0; i < wq_size; ++i){
                //     if(fdarr[i].fd != testfdarr[i].fd){
                //         printf("===fdarr[%d].fd and test.fd didnt match!\n", i);
                //         printf("===fdarr[%d].fd is %d, while test.fd = %d\n", i, fdarr[i].fd, testfdarr[i].fd);
                //     }
                //     if(fdarr[i].events != testfdarr[i].events){
                //         printf("===fdarr[%d].events and test.events didnt match!\n", i);
                //         printf("===fdarr[%d].events is %d, while test.events = %d\n", i, fdarr[i].events, testfdarr[i].events);
                //     }
                // }

                // fprintf(stderr, "start blocking polling\n");
                while(true){
                    if(poll(fdarr, wq_size, -1) < 0){
                        if(errno == EINTR) //errno is set to EINTR when some system calls are interrupted and the system is not in a position to resume the system call after interrupt handling.
                            continue;
                    }
                    else{
                        break;
                    }
                }
                // fprintf(stderr, "move finished waiting queue to ready queue\n");
                for(int i = 0; i < wq_size; ++i){
                    if(fdarr[i].revents & POLLIN){ //ready 2 write
                        // fprintf(stderr, "=id: %d ready to read\n", waiting_queue[i]->id);
                        ready_queue[rq_size] = waiting_queue[i];
                        ++rq_size;
                        waiting_queue[i] = NULL;
                    }
                }
                newWQ_size = 0;
                // fprintf(stderr, "renew waiting queue | FOREVER BLOCK\n");
                for(int i = 0; i < wq_size; ++i){
                    if(waiting_queue[i] == NULL){
                        memset(&fdarr+i, 0, sizeof(struct pollfd));
                        int j = i+1;
                        for(; j < wq_size; ++j){
                            if(waiting_queue[j] != NULL){
                                break;
                            }
                        }
                        if(j < wq_size){
                            waiting_queue[newWQ_size] = waiting_queue[j];
                            fdarr[newWQ_size] = fdarr[j];
                            memset(&fdarr+j, 0, sizeof(struct pollfd));
                            ++newWQ_size;
                        }
                        i = j;
                    }
                    else{
                        waiting_queue[newWQ_size] = waiting_queue[i];
                        fdarr[newWQ_size] = fdarr[i];
                        if(newWQ_size != i){
                            memset(&fdarr+i, 0, sizeof(struct pollfd));
                        }
                        ++newWQ_size;
                    }
                }
                // fprintf(stderr, "old_wq_size = %d, new_wq_size = %d | FOREVER BLOCK\n", wq_size, newWQ_size);
                wq_size = newWQ_size;

                // fprintf(stderr, "finally renew waiting queue, next is to set current = 0\n");
                rq_current = 0;
            }
            else{
                // fprintf(stderr, "RETURN TO MAIN\n");
                return;
            }
        }

        // fprintf(stderr, "jump to id: %d, rq_current: %d w/ rq_size = %d\n", RUNNING->id, rq_current, rq_size);
        longjmp(RUNNING->environment, -1);
    }
}
