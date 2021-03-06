//Autor: Arthur Martinelli Antonietto
//GRR20182559

// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.1 -- Julho de 2016

// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include <ucontext.h>		// biblioteca POSIX de trocas de contexto
#include "queue.h"		// biblioteca de filas genéricas

//Defines

#define STACKSIZE 64*1024	/* tamanho de pilha das threads */

// Estrutura que define um Task Control Block (TCB)
typedef struct task_t
{
   struct task_t *prev, *next ;		// ponteiros para usar em filas
   int id ;				// identificador da tarefa
   ucontext_t context ;			// contexto armazenado da tarefa
   int static_priority;
   int dynamic_priority;
   int system_task;
   int time_init;
   int time_processor;
   int activations;
   struct task_t *tasks_waiting_queue;
   int join_exit_code;
   int wake_up_time;
   // ... (outros campos serão adicionados mais tarde)
} task_t ;

// estrutura que define um semáforo
typedef struct
{
    int counter;
    queue_t **q;
} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  // preencher quando necessário
} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct
{
    void *buffer;
    int start, end, msg_size, queue_size;
    semaphore_t *s_item, *s_vaga, *s_mutex;
} mqueue_t ;

#endif

