#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"
#include "queue.h"

typedef struct item_t {

    struct item_t *prev, *next;
    int value;

} item_t;

task_t      produtor1, produtor2, produtor3, consumidor1, consumidor2;
semaphore_t s_item, s_vaga, s_mutex;
queue_t     *buffer=NULL;

void produtor (void * arg) {

    while(1) {
        task_sleep(1000);

        item_t *item;
        item = (item_t*)malloc(sizeof(item_t*));
        item->value = rand() % 100;
        printf("%s produziu %d(%d)\n", (char*) arg, item->value, queue_size(buffer));

        sem_down(&s_vaga);
        sem_down(&s_mutex);

        queue_append((queue_t**) &buffer, (queue_t*) item);

        sem_up(&s_mutex);
        sem_up(&s_item);

    }

   task_exit (0) ;

}

void consumidor (void * arg) {

    while(1) {

        sem_down(&s_item);
        sem_down(&s_mutex);

        item_t *item = (item_t*)buffer;
        queue_remove((queue_t**) &buffer, (queue_t*) item);

        sem_up(&s_mutex);
        sem_up(&s_vaga);

        printf("%s consumiu %d(%d)\n", (char*) arg, item->value, queue_size(buffer));

        task_sleep(1000);

    }

   task_exit (0) ;

}

int main (int argc, char *argv[])
{

   ppos_init () ;

   // cria semaforos
   sem_create (&s_item, 0) ;
   sem_create (&s_vaga, 5) ;
   sem_create (&s_mutex, 1) ;

   // cria tarefas
   task_create (&produtor1, produtor, "P1") ;
   task_create (&produtor2, produtor, "  P2") ;
   task_create (&produtor3, produtor, "    P3") ;
   task_create (&consumidor1, consumidor, "                         C1") ;
   task_create (&consumidor2, consumidor, "                             C2") ;

   task_exit (0) ;

}
