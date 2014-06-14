#include "scheduler.H"

extern FramePool* SYSTEM_FRAME_POOL;

struct myQueue{

    int head;
    int tail;
    Thread** Array;
};


Scheduler::Scheduler(){
    Queue q ;
    q->Array=(Thread**)(SYSTEM_FRAME_POOL->get_frame());
    q->head=0;
    q->tail=0;

    ready_queue=q;
}

void Scheduler::yield(){
    Thread** x=ready_queue->Array;
    Thread* y=x[ready_queue->head];
    ready_queue->head++;

    Thread* new_thread=y;
    Thread::dispatch_to(new_thread);
}

void Scheduler::resume(Thread *_thread){
    ready_queue->Array[ready_queue->tail]=_thread;
    ready_queue->tail++;
}

void Scheduler::add(Thread *_thread){
    ready_queue->Array[ready_queue->tail]=_thread;
    ready_queue->tail++;
}

void Scheduler::terminate(Thread *_thread){
    SYSTEM_FRAME_POOL->release_frame((unsigned long) _thread);
}













