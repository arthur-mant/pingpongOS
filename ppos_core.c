#include <stdio.h>
#include <stdlib.h>
#include "ppos_data.h"
#include "queue.h"

//#define DEBUG

//definições de variáveis globais
int task_count=0;
int user_tasks=0;
task_t *task_atual;
task_t task_main, task_dispatcher;
task_t *ready_task_queue=NULL;

void ppos_init() {

    setvbuf(stdout, 0, _IONBF, 0);

    task_main.id = 0;
    getcontext(&task_main.context);
    task_atual = &task_main;

    task_create(&task_dispatcher, dispatcher, NULL);

    #ifdef DEBUG
    printf("inicialização completa com sucesso\n");
    #endif

    task_yield();

}

int task_create (task_t *task, void (*start_routine)(void *), void *arg) {

    if (getcontext(&task->context))
        return -1;

    char *stack;
    stack = malloc (STACKSIZE) ;
    if (stack)
    {
        task->context.uc_stack.ss_sp = stack ;
        task->context.uc_stack.ss_size = STACKSIZE ;
        task->context.uc_stack.ss_flags = 0 ;
        task->context.uc_link = 0 ;
    }
    else
    {
        printf("erro na criação da pilha:\n");
        return -2;
    }

    makecontext(&task->context , (void*)(start_routine), 1, arg);
    task->id = ++task_count;

    if (task != &task_dispatcher) {     //contador de tarefas do usuário
        ++user_tasks;
        queue_append((queue_t **) &ready_task_queue, (queue_t*) task);

    #ifdef DEBUG
    printf("task_create: tarefa %d criada\n", task->id);
    #endif

    return task->id;

}

int task_switch (task_t *task) {

    #ifdef DEBUG
    printf("task_switch: trocando da tarefa %d para %d\n", task_atual->id, task->id);
    #endif
    task_t *task_anterior = task_atual;
    task_atual = task;
    if (swapcontext(&task_anterior->context, &task->context))
        return -1;
    return 0;

}

void task_exit (int exit_code) {

    if (task_atual == &task_dispatcher) 
        task_switch(&task_main);
    else
        user_tasks--;
        queue_remove((queue_t**) &ready_task_queue, (queue_t*) task_atual);
        task_switch(&task_dispatcher);
}

int task_id () {

    return task_atual->id;

}

void task_yield() {
    task_switch(&task_dispatcher);
}

void dispatcher() {
    task_t *proxima;
    while (user_tasks > 0) {
        proxima = scheduler()
        if (proxima != NULL) {
            task_switch(proxima);
        }
    }
    task_exit(0);
}

task_t *scheduler() {
    if (ready_task_queue == NULL)
        return NULL
    task_t *first_task=ready_task_queue;
    ready_task_queue = ready_task_queue->next;
    return first_task;
}
