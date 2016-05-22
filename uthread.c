#ifndef MY_UTHREAD_CPP
#define MY_UTHREAD_CPP


#include "uthread.h"

void uthread_resume(schedule_t *schedule , int id){
    if(id < 0 || id >= Max_Thread_SIZE){
        return;
    }

    uthread_t *t = &(schedule->threads[id]);

    switch(t->state){
        case RUNNABLE:
            getcontext(&(t->ctx));
    
            t->ctx.uc_stack.ss_sp = t->stack;
            t->ctx.uc_stack.ss_size = DEFAULT_STACK_SIZE;
            t->ctx.uc_stack.ss_flags = 0;
            t->ctx.uc_link = &(schedule->main);
            t->state = RUNNING;

            schedule->running_thread = id;

            makecontext(&(t->ctx),(void (*)(void))(uthread_body),1,schedule);
            swapcontext(&(schedule->main),&(t->ctx));
            /* !! note : Here does not need to break */

        case SUSPEND:
            schedule->running_thread = id;
            swapcontext(&(schedule->main),&(t->ctx));

            break;
        default:  perror("resume thread failed,unsupported thread state!");
    }
}

void uthread_yield(schedule_t *schedule){
    if(schedule->running_thread != -1 ){
        uthread_t *t = &(schedule->threads[schedule->running_thread]);
        t->state = SUSPEND;
        schedule->running_thread = -1;

        swapcontext(&(t->ctx),&(schedule->main));
    }else{
        perror("can't yield while no threads running!");
    }
}

void *uthread_getArg(schedule_t *schedule){
     if(schedule->running_thread != -1 ){
        uthread_t *t = &(schedule->threads[schedule->running_thread]);
        return t->arg;
     }else{
        perror("can't get argument while no threads running!");
    }
}

int uthread_getId(schedule_t *schedule){
     if(schedule->running_thread != -1 ){
        return schedule->running_thread;
     }else{
        perror("can't get argument while no threads running!");
    }
    return -1;
}


void uthread_body(schedule_t *schedule){
    int id = schedule->running_thread;

    if(id != -1){
        uthread_t *t = &(schedule->threads[id]);

        t->func(schedule);
        free(uthread_getArg(schedule));
        t->state = FREE;
        schedule->running_thread = -1;
    }
}

int uthread_create(schedule_t *schedule,Fun func,void *arg){
    int id = 0;
    
    for(id = 0; id < Max_Thread_SIZE; ++id ){
        if(schedule->threads[id].state == FREE){
            break;
        }
    }    
    uthread_t *t = &(schedule->threads[id]);

    t->state = RUNNABLE;
    t->func = func;
    t->arg = arg;

    return id;
}

int schedule_finished( schedule_t *schedule){
    if (schedule->running_thread != -1){
        return 0;
    }else{
        for(int i = 0; i < Max_Thread_SIZE; ++i){
            if(schedule->threads[i].state != FREE){
                return 0;
            }
        }
    }

    return 1;
}

#endif
