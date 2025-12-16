#include "array.h"
#include <string.h>
#include <stdlib.h>
#include <semaphore.h>


#include <stdio.h>

int array_init(array *s) {
    s->top = 0;
    sem_init(&s->sem_array_full, 0, 0);
    sem_init(&s->sem_array_empty, 0, 8);//match array size
    sem_init(&s->sem_array_lock, 0, 1);

    return 0;
}


int array_put(array *s, char *hostname) {

//Lock
    sem_wait(&(s->sem_array_empty));
    sem_wait(&(s->sem_array_lock));
//Critical Section Begins
    *(s->array[s->top]) = hostname; 
    s->top = s->top + 1;
//Critical Section Ends
//Unlock
    sem_post(&(s->sem_array_lock));
    sem_post(&(s->sem_array_full));

   return 0;
}


int array_get(array *s, char **hostnames) {    
//Lock
    sem_wait(&(s->sem_array_full));
    sem_wait(&(s->sem_array_lock));
//Critical Section Begins
    *hostnames = *(s->array[s->top - 1]);
    s->top = s->top - 1;
//Critical Section Ends
//Unlock
    sem_post(&(s->sem_array_lock));
    sem_post(&(s->sem_array_empty));


    return 0;

}

void array_free(array *s) {
    sem_destroy(&(s->sem_array_lock));
    sem_destroy(&(s->sem_array_empty));
    sem_destroy(&(s->sem_array_full));
}


//gcc -o test_array test_array.c array.c -lpthread
