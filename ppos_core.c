#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include "ppos_data.h"
#include "queue.h"

//DEFINES
//#define DEBUG
#define QUANTUM 20 //em ms

//definições de variáveis globais
int task_count=0;
int user_tasks=0;
int quantum_counter;
task_t *task_atual;
task_t task_main, task_dispatcher;
task_t *ready_task_queue=NULL;

struct sigaction action;
struct itimerval timer;

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

    if (!task->system_task) {     //contador de tarefas do usuário
        ++user_tasks;
        queue_append((queue_t **) &ready_task_queue, (queue_t*) task);
    }

    #ifdef DEBUG
    printf("task_create: tarefa %d criada na memória %p\n", task->id, task);
    #endif

    return task->id;

}

int task_switch (task_t *task) {

    #ifdef DEBUG
    printf("task_switch: trocando da tarefa %d para %d\n", task_atual->id, task->id);
    #endif
    task_t *task_anterior = task_atual;
    task_atual = task;
    quantum_counter = QUANTUM;
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
    #ifdef DEBUG
    printf("scheduler inicializado\n");
    #endif
    if (ready_task_queue == NULL)
        return NULL;

    int aging_value = -1;
    task_t *aux_task, *priority_task;
    priority_task = ready_task_queue;
    aux_task = ready_task_queue->next;

    while (aux_task != ready_task_queue) {
    
        if (priority_task->dynamic_priority > aux_task->dynamic_priority) {
            
            priority_task->dynamic_priority += aging_value;
            priority_task = aux_task;

        }
        else {

            aux_task->dynamic_priority += aging_value;

        }
        
        aux_task = aux_task->next;
    }
    priority_task->dynamic_priority = priority_task->static_priority;
    #ifdef DEBUG
    printf("a tarefa %d possui a maior prioridade\n", priority_task->id);
    #endif

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

void preemptor (int signum) {

    if (task_atual->system_task)
        return;

    quantum_counter--;
    if (quantum_counter == 0)
        task_yield();

}

void ppos_init() {

    setvbuf(stdout, 0, _IONBF, 0);

    task_main.id = 0;
    task_main.system_task = 1;
    getcontext(&task_main.context);

    task_atual = &task_main;

    task_dispatcher.system_task = 1;
    task_create(&task_dispatcher, dispatcher, NULL);

    //configurando o preemptor
    action.sa_handler = preemptor;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    if (sigaction(SIGALRM, &action, 0) < 0) {
        perror("Erro em sigaction: ");
        exit(1);
    }

    //configurando o timer
    timer.it_value.tv_usec = 1000;
    timer.it_value.tv_sec = 0;
    timer.it_interval.tv_usec = 1000;
    timer.it_interval.tv_sec = 0;

    if (setitimer(ITIMER_REAL, &timer, 0) < 0) {
        perror("erro em setitimer: ");
        exit(1);
    }

    #ifdef DEBUG
    printf("inicialização completa com sucesso\n");
    #endif

}
