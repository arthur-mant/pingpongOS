#include <stdio.h>
#include <stdlib.h>
#include "ppos_data.h"

int task_count=0;
task_t *task_atual;
task_t task_main;

void ppos_init() {

    setvbuf(stdout, 0, _IONBF, 0);

    task_main.id = 0;
    getcontext(&task_main.context);
    task_atual = &task_main;

    printf("inicialização completa com sucesso\n");

}

void hi () {
    printf("hi\n");
}

int task_create (task_t *task, void (*start_routine)(void *), void *arg) {

    printf("tentando pegar contexto\n");
    if (getcontext(&task->context))
        return -1;

    printf("criando pilha\n");
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

    printf("criando contexto\n");
    makecontext(&task->context , (void*)(start_routine), 1, arg);
    printf("contexto criado\n");
    task->id = ++task_count;

    return task->id;

}

int task_switch (task_t *task) {

    printf("task atual: %d\n", task_atual->id);
    task_t *task_anterior = task_atual;
    task_atual = task;
    if (swapcontext(&task_anterior->context, &task->context))
        return -1;
    return 0;

}

void task_exit (int exit_code) {

    task_switch (&task_main);

}

int task_id () {

    return task_atual->id;

}
