#include "threadtools.h"

void fibonacci(int id, int arg) {
    thread_setup(id, arg);

    for (RUNNING->i = 1; ; RUNNING->i++) {
        if (RUNNING->i <= 2)
            RUNNING->x = RUNNING->y = 1;
        else {
            /* We don't need to save tmp, so it's safe to declare it here. */
            int tmp = RUNNING->y;  
            RUNNING->y = RUNNING->x + RUNNING->y;
            RUNNING->x = tmp;
        }
        printf("%d %d\n", RUNNING->id, RUNNING->y);
        sleep(1);

        if (RUNNING->i == RUNNING->arg) {
            thread_exit();
        }
        else {
            thread_yield();
        }
    }
}

void collatz(int id, int arg) {
    // TODO
    thread_setup(id, arg);

    while(RUNNING->arg > 0){
        if(RUNNING->arg % 2){ //odd
            RUNNING->arg *= 3; ++RUNNING->arg;
            printf("%d %d\n", RUNNING->id, RUNNING->arg);
        }
        else{ //even
            RUNNING->arg /= 2;
            printf("%d %d\n", RUNNING->id, RUNNING->arg);
        }
        sleep(1);

        if(RUNNING->arg == 1){
            break;
        }
        else{
            thread_yield();
        }
    }
    thread_exit();
}

void max_subarray(int id, int arg) {
    // TODO
    thread_setup(id, arg);

    //ref: https://shubo.io/maximum-subarray-problem-kadane-algorithm/
    //i: result, x: max_ending_here, y: new number
    RUNNING->i = 0; RUNNING->x = 0;
    bool neg;
    char cur;

    while(RUNNING->arg){
        --RUNNING->arg;
        async_read(sizeof(char)*5);
        //parse input data
        neg = false;
        RUNNING->y = 0;
        for(int idx = 0; idx < 4; ++idx){
            cur = RUNNING->buf[idx];
            if(cur == ' '){
                continue;
            }
            else{
                if(cur == '-'){
                    neg = true;
                }
                else if(0 <= (int)(cur - '0') && (int)(cur - '0') <= '9'){
                    RUNNING->y = RUNNING->y * 10 + (int)(cur - '0');
                }
            }
        }
        if(neg){
            RUNNING->y = -RUNNING->y;
        }
        // fprintf(stderr, "get number %d\n", RUNNING->y);
        
        RUNNING->x = (RUNNING->x + RUNNING->y > 0)? RUNNING->x + RUNNING->y : 0; //max_ending_here = max(0, max_ending_here + a)
        if(RUNNING->x > RUNNING->i){  //result = max(result, max_ending_here)
            RUNNING->i = RUNNING->x;
        }
        printf("%d %d\n", RUNNING->id, RUNNING->i);
        sleep(1);

        if(RUNNING->arg == 0){
            break;
        }
        else{
            thread_yield();
        }
    }
    thread_exit();
}
