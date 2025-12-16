#ifndef ARRAY_H //include guard
#define ARRAY_H
#include <semaphore.h>

#define ARRAY_SIZE 8 
#define MAX_NAME_LENGTH 30
typedef struct {
    char *array[ARRAY_SIZE][MAX_NAME_LENGTH];
    int top;
    sem_t sem_array_full;
    sem_t sem_array_empty;
    sem_t sem_array_lock;
} array;



int array_init(array *s); //init the array
int array_put(array *s, char *hostname); //place elment on the top of the array
int array_get(array *s, char **hostame); //remove element from the top of the array
void array_free(array *s);             //free the array's recources

#endif
