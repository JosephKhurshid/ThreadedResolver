//CITATION: The following code is heavily adapted from Professor Herman's code black in PA4

#ifndef MULTI_LOOKUP_H //include guard
#define MULTI_LOOKUP_H
#include <semaphore.h>
#include <pthread.h>
#include "array.h"


#define MAX_NAME_LENGTH 30
#define MAX_INPUT_FILES 100  
#define MAX_OUTPUT_FILES 2  
#define MAX_REQUESTER_THREADS 10
#define MAX_RESOLVER_THREADS 10
#define MAX_IP_LENGTH 46


typedef struct {
    pthread_t requester_threads_array[MAX_REQUESTER_THREADS];
    pthread_t resolver_threads_array[MAX_RESOLVER_THREADS];
    char file_names[MAX_INPUT_FILES][MAX_NAME_LENGTH];
    char output_file_names[MAX_OUTPUT_FILES][MAX_NAME_LENGTH];
    int current_file_index;
    int total_amount_of_files;
    int files_read;
    int requestor_threads_done;
    int array_put_amount;
    int array_get_amount;
    int top_of_requester_threads;
    int top_of_resolver_threads;
    array hostname_array;
    FILE *file_pointer_for_requester_file;
    FILE *file_pointer_for_resolver_file;
    sem_t lock_accesing_input_files;
    sem_t lock_requestor_file;
    sem_t lock_resolver_file;
    sem_t lock_incrementing_array_put_amount;
    sem_t lock_incrementing_array_get_amount;
} multi_lookup_struct;



int multi_lookup_init(multi_lookup_struct *t, int req_threads_amount, int res_threads_amount, int argc, char* argv[]); //init the multi lookup struct
int create_all_threads(multi_lookup_struct *t, int amount_of_req_threads, int amount_of_res_threads);// create all threads
int store_host_file_name(multi_lookup_struct *t, char file_name[]); //function to store file names into the struct char array
int store_output_file_name(multi_lookup_struct *t, char file_name[], int index);// function to store output file name
void* request_hostnames_from_files(void *t);
void* reslove_hostnames_from_files(void *t);
void multi_lookup_free(multi_lookup_struct *t);//free the multi lookup struct's recources

#endif
