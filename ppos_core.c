//Autor: Arthur Martinelli Antonietto
//GRR20182559

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>
#include "ppos_data.h"
#include "queue.h"

//DEFINES
//#define DEBUG
#define QUANTUM 20 //em ms

//definições de variáveis globais
int task_count=0;
int user_tasks=0;
int quantum_counter;
int busy_waiting_lock=0;
unsigned int real_time=0;
unsigned int task_atual_init_time=0;
task_t *task_atual;
task_t task_main, task_dispatcher;
task_t *ready_task_queue=NULL;
task_t *sleeping_task_queue=NULL;

struct sigaction action;
struct itimerval timer;

unsigned int systime() {
    return real_time;
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
    task->static_priority = task->dynamic_priority = 0;
    task->time_init = systime();
    task->time_processor = 0;
    task->activations = 0;

    if (task->system_task == 0) {     //contador de tarefas do usuário
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
    //printf("task_switch: trocando da tarefa %d para %d\n", task_atual->id, task->id);
    #endif

    task_atual->time_processor += systime()-task_atual_init_time;

    task_t *task_anterior = task_atual;
    task_atual = task;
    quantum_counter = QUANTUM;

    task->activations++;
    task_atual_init_time = systime();

    if (swapcontext(&task_anterior->context, &task->context))
        return -1;
    return 0;

}

void task_exit (int exit_code) {

    printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations\n",
        task_atual->id,
        systime()-task_atual->time_init,
        task_atual->time_processor+systime()-task_atual_init_time,
        task_atual->activations
    );

    if (task_atual != &task_dispatcher) {
        user_tasks--;
        queue_remove((queue_t**) &ready_task_queue, (queue_t*) task_atual);

        if (task_atual->tasks_waiting_queue != NULL) {

            task_t *aux_task = task_atual->tasks_waiting_queue;

            while(queue_size((queue_t*)task_atual->tasks_waiting_queue) != 0) {
                queue_remove((queue_t **) &task_atual->tasks_waiting_queue, (queue_t*) aux_task);
                queue_append((queue_t **) &ready_task_queue, (queue_t*) aux_task);
                aux_task->join_exit_code = exit_code;
                aux_task = task_atual->tasks_waiting_queue;
            }
        }

        task_switch(&task_dispatcher);
    }
}

void task_yield() {
    task_switch(&task_dispatcher);
}

void task_sleep(int sleep_time) {

    task_atual->wake_up_time = real_time+sleep_time;
    queue_remove((queue_t **) &ready_task_queue, (queue_t*) task_atual);
    queue_append((queue_t **) &sleeping_task_queue, (queue_t*) task_atual);
    #ifdef DEBUG
    printf("task_sleep: colocando task %d para dormir %d ms\n", task_atual->id, sleep_time);
    #endif

    task_yield();
    

}

task_t *find_in_queue(task_t *task, task_t *queue) {

    if (queue == NULL)
        return NULL;

    task_t *aux_task=queue;
    task_t *aux_child_task=aux_task->tasks_waiting_queue;
    int task_counter=0;

    while (
        (task_counter < queue_size((queue_t*)queue)) &&
        (aux_task != task) &&
        (aux_child_task != task)
        ) {

        aux_child_task = find_in_queue(task, aux_task->tasks_waiting_queue);

        task_counter++;
        aux_task = aux_task->next;
    }

    if ((aux_task == task) || (aux_child_task == task))
        return task;
    else
        return NULL;

}

int task_join (task_t *task) {

    if ((find_in_queue(task, ready_task_queue) == NULL) && 
        (find_in_queue(task, sleeping_task_queue) == NULL)){
        #ifdef DEBUG
        printf("task_join: tarefa %d não encontrada\n", task->id);
        #endif
        return -1;
    }

    #ifdef DEBUG
    printf("task_join: removendo tarefa %d da fila de prontas e adicionando na fila de espera da tarefa %d\n", task_atual->id, task->id);
    #endif
    queue_remove((queue_t**) &ready_task_queue, (queue_t*) task_atual);
    queue_append((queue_t**)&(task->tasks_waiting_queue), (queue_t*)task_atual);

    task_yield();

    return task_atual->join_exit_code;

}

int task_id () {

    return task_atual->id;

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
    //printf("scheduler inicializado\n");
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
    //printf("a tarefa %d possui a maior prioridade\n", priority_task->id);
    #endif

    return priority_task;
}

void dispatcher() {
    #ifdef DEBUG
    printf("dispatcher inicializado\n");
    #endif
    task_t *proxima;
    while (user_tasks > 0) {

        task_t *aux_task, *aux_next;
        int task_counter;
        aux_task = sleeping_task_queue;
        task_counter = queue_size((queue_t *)sleeping_task_queue);
        while (task_counter > 0) {
            aux_next = aux_task->next;
            if (aux_task->wake_up_time <= real_time) {
                #ifdef DEBUG
                printf("dispatcher: acordande %d em %d ms\n", aux_task->id, real_time);
                #endif
                queue_remove((queue_t **) &sleeping_task_queue, (queue_t*) aux_task);
                queue_append((queue_t **) &ready_task_queue, (queue_t*) aux_task);
            }

            task_counter--;
            aux_task = aux_next;

        }

        proxima = scheduler();
        if (proxima != NULL) {
            task_switch(proxima);
        }
    }
    task_exit(0);
}

void preemptor (int signum) {

    real_time++;

    if (task_atual->system_task == 1) {
        #ifdef DEBUG
        printf("preempção em tarefa de sistema, retornando\n");
        #endif
        return;
    }

    quantum_counter--;
    if (quantum_counter == 0) {
        #ifdef DEBUG
    //    printf("interrompendo execução da tarefa %d\n", task_atual->id);
        #endif
        task_yield();
    }

}

void ppos_init() {

    setvbuf(stdout, 0, _IONBF, 0);

    task_main.id = 0;
    task_main.static_priority = task_main.dynamic_priority = 0;
    task_main.time_init = 0;
    task_main.time_processor = 0;
    task_main.activations = 0;
    getcontext(&task_main.context);

    task_atual = &task_main;
    ++user_tasks;
    queue_append((queue_t **) &ready_task_queue, (queue_t*) &task_main);

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

    task_yield();

}

int sem_create (semaphore_t *s, int value) {

    if (s == NULL)
        return -1;

    s->counter = value;
    s->q = malloc(sizeof(queue_t *));

    #ifdef DEBUG
    printf("semaphore created\n");
    #endif

    return 0;

}

int sem_down (semaphore_t *s) {

    if ((s == NULL) || (s->q == NULL))
        return -1;

    #ifdef DEBUG
    printf("sem_down started: busy waiting\n");
    #endif
    while (__sync_fetch_and_or(&busy_waiting_lock,1));

    s->counter -= 1;
    if (s->counter < 0) {
        queue_remove((queue_t **) &ready_task_queue, (queue_t*) task_atual);
        queue_append(s->q, (queue_t *) task_atual);
        busy_waiting_lock=0;
        task_yield();
    }
    busy_waiting_lock=0;
    #ifdef DEBUG
    printf("lock set to 0\n");
    #endif
    return 0;
}

int sem_up (semaphore_t *s) {

    if ((s == NULL) || (s->q == NULL))
        return -1;

    #ifdef DEBUG
    printf("sem_up started: busy waiting\n");
    #endif

    while (__sync_fetch_and_or(&busy_waiting_lock,1));

    s->counter += 1;
    if (s->counter <= 0) {
        queue_t *aux_task = *(s->q);
        queue_remove(s->q, (queue_t*) aux_task);
        queue_append((queue_t **) &ready_task_queue, (queue_t *) aux_task);
    }
    busy_waiting_lock=0;
    #ifdef DEBUG
    printf("lock set to 0\n");
    #endif
    return 0;
}

int sem_destroy (semaphore_t *s) {

    if ((s == NULL) || (s->q == NULL))
        return -1;

    queue_t *aux_task = *(s->q);
    while (*(s->q) != NULL) {
        queue_remove(s->q, aux_task);
        queue_append((queue_t **) &ready_task_queue, aux_task);
        aux_task = *(s->q);
    }
    s->q=NULL;
    #ifdef DEBUG
    printf("semaphore_destroyed\n");
    #endif

    return 0;
}

int mqueue_create (mqueue_t *queue, int max_msgs, int msg_size) {

    if (queue == NULL)
        return -1;
    //o tamanho da fila é 1 a mais q o maximo de mensagens para ser possível diferenciar
    //uma fila cheia de uma fila vazia
    queue->buffer = (void *)malloc((max_msgs+1) * msg_size);

    semaphore_t *s_item, *s_vaga, *s_mutex;

    s_item = malloc(sizeof(semaphore_t));
    s_vaga = malloc(sizeof(semaphore_t));
    s_mutex = malloc(sizeof(semaphore_t));

    if (sem_create(s_item, 0) == -1)
        return -1;
    if (sem_create(s_vaga, msg_size) == -1)
        return -1;
    if (sem_create(s_mutex, 1) == -1)
        return -1;

    queue->s_item = s_item;
    queue->s_vaga = s_vaga;
    queue->s_mutex = s_mutex;

    queue->start = queue->end = 0;
    queue->msg_size = msg_size;
    queue->queue_size = max_msgs+1;

    return 0;

}

int mqueue_send (mqueue_t *queue, void *msg) {

    if (queue == NULL)
        return -1;

    if (sem_down(queue->s_vaga) == -1)
        return -1;
    if (sem_down(queue->s_mutex) == -1)
        return -1;

    memcpy((queue->buffer + queue->end * queue->msg_size), msg, queue->msg_size);
    queue->end = (queue->end+1) % queue->queue_size;

    if (sem_up(queue->s_mutex) == -1)
        return -1;
    if (sem_up(queue->s_item) == -1)
        return -1;

    return 0;

}

int mqueue_recv (mqueue_t *queue, void *msg) {

    if (queue == NULL)
        return -1;

    if (sem_down(queue->s_item) == -1)
        return -1;
    if (sem_down(queue->s_mutex) == -1)
        return -1;

    memcpy(msg, (queue->buffer + queue->start * queue->msg_size), queue->msg_size);
    queue->start = (queue->start+1) % queue->queue_size;

    if (sem_up(queue->s_mutex) == -1)
        return -1;
    if (sem_up(queue->s_vaga) == -1)
        return -1;

    return 0;

}

int mqueue_destroy (mqueue_t *queue) {

    if (sem_destroy(queue->s_item) == -1)
        return -1;
    if (sem_destroy(queue->s_vaga) == -1)
        return -1;
    if (sem_destroy(queue->s_mutex) == -1)
        return -1;
    free(queue->buffer);
    queue = NULL;

    return 0;
}

int mqueue_msgs (mqueue_t *queue) {

    if (queue == NULL)
        return -1;
    int result = (queue->end - queue->start);
    if (result < 0)
        result+=queue->queue_size;
    return result;

}
