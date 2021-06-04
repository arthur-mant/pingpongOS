//Autor: Arthur Martinelli Antonietto
//GRR20182559

#include <stdio.h>

typedef struct queue_t
{
   struct queue_t *prev ;  // aponta para o elemento anterior na fila
   struct queue_t *next ;  // aponta para o elemento seguinte na fila
} queue_t ;

int queue_size (queue_t *queue) {
    int size = 1;
    queue_t *aux;

    if (queue == NULL)
        return 0;
    aux = queue->next;
    while (aux != queue) {
        size++;
        aux = aux->next;
    }
    return size;
}

void queue_print (char *name, queue_t *queue, void print_elem (void*) ) {
    queue_t *aux;
    
    printf("%s : [", name);
    if (queue != NULL) {    //se a fila não estiver vazia
        print_elem(queue);
        printf(" ");

        aux = queue->next;
        while (aux != queue) {
            print_elem(aux);
            printf(" ");
            aux = aux->next;
        }
    }
    printf("]\n");
}

int queue_append (queue_t **queue, queue_t *elem) {
    if (queue == NULL) {
        printf("A fila não existe\n");
        return -1;
    }
    if (elem == NULL) {
        printf("O elemento não existe\n");
        return -2;
    }
    if ((elem->next != NULL) || (elem->prev != NULL)) {
        printf("O elemento pertence a outra fila\n");
        return -3;
    }

    queue_t *first;

    first = *queue;
 
    if (first == NULL) {
        elem->prev = elem;
        elem->next = elem;
        *queue = elem;
    }

    else {
        elem->next = first;
        elem->prev = first->prev;
        first->prev->next = elem;
        first->prev = elem;
    }

    return 0;

}

int queue_remove (queue_t **queue, queue_t *elem) {
    if (queue == NULL) {
        printf("A fila não existe\n");
        return -1;
    }
    if (*queue == NULL) {
        printf("A fila está vazia\n");
        return -2;
    }
    if (elem == NULL) {
        printf("O elemento não existe\n");
        return -3;
    }

    queue_t *aux;
    int tam, i=0;

    tam = queue_size(*queue);
    aux = *queue;
    while ((aux != elem) && (i < tam)) {
        aux = aux->next;
        i++;
    }
    if (i == tam) {
        printf("O elemento não pertence à fila\n");
        return -4;
    }

    if (aux == *queue) {        //se for o primeiro elemento
        if (aux->next == aux)   //se for o unico elemento
            *queue = NULL;
        else
            *queue = aux->next;
    }
    aux->next->prev = aux->prev;
    aux->prev->next = aux->next;
    aux->prev = NULL;
    aux->next = NULL;

    return 0;
}

