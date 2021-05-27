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
    task->static_priority = task->dynamic_priority = 0;

    if (task != &task_dispatcher) {     //contador de tarefas do usuário
        ++user_tasks;
        queue_append((queue_t **) &ready_task_queue, (queue_t*) task);
    }

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
    else {
        user_tasks--;
        queue_remove((queue_t**) &ready_task_queue, (queue_t*) task_atual);
        task_switch(&task_dispatcher);
    }
}

int task_id () {

    return task_atual->id;

}

void task_yield() {
    task_switch(&task_dispatcher);
}

void task_setprio(task_t *task, int prio) {

    if ((prio > 20) || (prio < -20))
        printf("ERRO: prioridade %d inválida", prio);
    else {
        if (task == NULL)
            task_atual->static_priority = task_atual->dynamic_priority = prio;
        else
            task->static_priority = task->dynamic_priority = prio;
    }
}

int task_getprio(task_t *task) {

    if (task == NULL)
        return task_atual->static_priority;
    return task->static_priority;

}

task_t *scheduler() {
    if (ready_task_queue == NULL)
        return NULL;

    int aging_value = -1;
    task_t *aux_task, priority_task;
    priority_task = ready_task_queue;
    aux_task = ready_task_queue->next;

    while (aux_task != ready_task_queue) {

        if (priority_task->dynamic_priority > aux_task->dynamic_priority) {

            priority_task->dynamic_priority += aging_value;
            priority_task = aux_task;

        }
        else {

            aux_task += aging_value;

        }
        aux_task = aux_task->next;

    }
    priority_task->dynamic_priority = priority_task->static_priority;

    return priority_task;
}

void dispatcher() {
    #ifdef DEBUG
    printf("dispatcher inicializado\n");
    #endif
    task_t *proxima;
    while (user_tasks > 0) {
        proxima = scheduler();
        if (proxima != NULL) {
            task_switch(proxima);
            //tratar a tarefa de acordo com o estado: pronta, terminada, etc.
        }
    }
    task_exit(0);
}


void ppos_init() {

    setvbuf(stdout, 0, _IONBF, 0);

    task_main.id = 0;
    getcontext(&task_main.context);
    task_atual = &task_main;

    task_create(&task_dispatcher, dispatcher, NULL);

    #ifdef DEBUG
    printf("inicialização completa com sucesso\n");
    #endif

}
