#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "array.h"
#include "multi-lookup.h"
#include "util.h"

int main(int argc, char* argv[] ) {

    if (argc >105) {
        fprintf(stderr, "%s", "Error: There should only be a max of a 100 input files.\n");
        return 1;
    }

    if (argc < 6) {
        fprintf(stderr, "%s", "Error: There should be a minimum of 5 arguments when calling the function.\n");
        return 1;

    }
    //Initialize my_mult_lookup_struct with the requester and resolver threads
    multi_lookup_struct my_mult_lookup_struct;
    multi_lookup_init(&my_mult_lookup_struct, atoi(argv[1]),atoi(argv[2]), argc, argv);
    multi_lookup_free(&my_mult_lookup_struct);

   return 0;
}

int store_host_file_name(multi_lookup_struct *t, char file_name[]){
    strcpy(t->file_names[t->current_file_index], file_name);
    t->current_file_index = t->current_file_index + 1;
    return 0;
}

int store_output_file_name(multi_lookup_struct *t, char file_name[], int index){
    strcpy( (t->output_file_names)[index], file_name);
    return 0;
}

void* request_hostnames_from_files(void *t){
    char hostname_line[50];
    char *hostname; 
    multi_lookup_struct *s = (multi_lookup_struct*) t; 
    FILE *file_pointer_for_input_file;
    int files_resolved = 0;

    while ( (s->files_read) < (s->total_amount_of_files) ){

        sem_wait(&(s->lock_accesing_input_files));//lock access to input files.
       
        if ( (s->files_read) < (s->total_amount_of_files) ) {// to account for changes in s->files_read while waiting
            file_pointer_for_input_file = fopen(s->file_names[s->files_read],"r");
            
            if (file_pointer_for_input_file == NULL) {//if file pointer errored, still increment
                fprintf(stderr, "Invalid file: %s\n", s->file_names[s->files_read]);

                s->files_read = s->files_read + 1;
                sem_post(&(s->lock_accesing_input_files));//unlock access to input files

            } else{
                s->files_read = s->files_read + 1;
                sem_post(&(s->lock_accesing_input_files));//unlock access to input files

                while (fgets(hostname_line, sizeof(hostname_line), file_pointer_for_input_file) != NULL) {

                    hostname = (char*) malloc(sizeof(char*)); //dont forget to free in resolver
                    int size = strlen(hostname_line);
                    hostname_line[size-1] = '\0';
                    strcpy(hostname, hostname_line);

                    if (hostname[0] != 0) {
                        array_put(&(s->hostname_array), hostname);//put hostname in array. Dynamically allocated to avoid pointers pointing to the same char array
                        
                        sem_wait(&(s->lock_incrementing_array_put_amount));
                        s->array_put_amount = s->array_put_amount + 1;
                        sem_post(&(s->lock_incrementing_array_put_amount)); 
                        
                        sem_wait(&(s->lock_requestor_file)); //Lock when writing in the requester file
                        sprintf(hostname_line, "%s\n", hostname);//concatenate the newline with the hostname
                        fputs(hostname_line, s->file_pointer_for_requester_file);//put the hostname_line in the file detailing all the input file hostnames
                        sem_post(&(s->lock_requestor_file)); //Unlock once done writing in the requestor file

                    }else{ //hostname equals the null character
                        free(hostname);//free the dynamically allocated memory that was assigned the null character

                    }
                files_resolved++;
                }
                fclose(file_pointer_for_input_file);

            }
        }
    
    }

    return 0;
}

void* reslove_hostnames_from_files(void *t){
    char output_line[100];
    char *single_hostname;
    char ip_string[MAX_IP_LENGTH] ;
    multi_lookup_struct *s = (multi_lookup_struct*) t; 
    int dns_result;
    int hostnames_resolved = 0;

    while (1){
            sem_wait(&(s->lock_incrementing_array_get_amount));//locking when checking array_put_amount
            if (s->requestor_threads_done & (s->array_get_amount == s->array_put_amount))// if requestors done and array empty 
            {
                sem_post(&(s->lock_incrementing_array_get_amount)); //Unlocked
                break;
            }
            sem_post(&(s->lock_incrementing_array_get_amount)); //Unlocked

            array_get(&s->hostname_array, &single_hostname);
            sem_wait(&(s->lock_incrementing_array_get_amount));
            s->array_get_amount = s->array_get_amount + 1;
            sem_post(&(s->lock_incrementing_array_get_amount)); 

            dns_result = dnslookup(single_hostname, ip_string, MAX_IP_LENGTH);

            if (dns_result !=0) {
                sem_wait(&(s->lock_resolver_file)); //Lock when writing in the requester file
                sprintf(output_line,"%s, %s\n", single_hostname, "NOT_RESOLVED"); //combine the hostame, a comma, the ip, and \n in a single line
                fputs(output_line, s->file_pointer_for_resolver_file);//put the output_line in the file detailing all the input file hostnames
                free(single_hostname);
                sem_post(&(s->lock_resolver_file)); //Unlock once done writing in the requestor file
            } else {
                sem_wait(&(s->lock_resolver_file)); //Lock when writing in the requester file
                sprintf(output_line,"%s, %s\n", single_hostname,ip_string); //combine the hostame, a comma, the ip, and \n in a single line
                fputs(output_line, s->file_pointer_for_resolver_file);//put the output_line in the file detailing all the input file hostnames
                free(single_hostname);
                sem_post(&(s->lock_resolver_file)); //Unlock once done writing in the requestor file
            }
        hostnames_resolved++;

        }

    return 0;
}

int multi_lookup_init(multi_lookup_struct *t, int req_threads_amount, int res_threads_amount, int argc, char* argv[]) {
    t->top_of_requester_threads = 0;
    t->top_of_resolver_threads = 0;
    t->current_file_index = 0;
    t->files_read = 0;
    t->requestor_threads_done = 0;
    t->array_put_amount = 0;
    t->array_get_amount = 0;
    FILE *file_pointer_for_requester_file;
    FILE *file_pointer_for_resolver_file;

    sem_init(&t->lock_requestor_file, 0, 1);
    sem_init(&t->lock_resolver_file, 0, 1);
    sem_init(&t->lock_accesing_input_files, 0, 1);
    sem_init(&t->lock_incrementing_array_put_amount, 0, 1);
    sem_init(&t->lock_incrementing_array_get_amount, 0, 1);
    
    t->total_amount_of_files = argc - 5;
    
    if (array_init(&t->hostname_array) < 0) {
        printf("The array initialization has errored");
        exit(-1);
    }
    
    for(int i = 3; i < 5; i = i+1){
        store_output_file_name(t, argv[i],i-3);//use store function to transfer file names to struct file_names
    }

    file_pointer_for_requester_file = fopen((t->output_file_names)[0], "a+");

    if (file_pointer_for_requester_file == NULL){
        fprintf(stderr, "Invalid output file for all serviced hostnames: %s\n", (t->output_file_names)[0]);
        return 1;
    }
    t->file_pointer_for_requester_file = file_pointer_for_requester_file;


    file_pointer_for_resolver_file = fopen((t->output_file_names)[1], "a+");
    if (file_pointer_for_resolver_file == NULL){
        fprintf(stderr, "Invalid output file for all resolved hostnames: %s\n", (t->output_file_names)[1]);
        return 1;
    }
     t->file_pointer_for_resolver_file = file_pointer_for_resolver_file;

    for(int i = 5; i < argc; i = i+1){
        store_host_file_name(t, argv[i]);//use store function to transfer file names to struct file_names
    }
    create_all_threads(t, req_threads_amount, res_threads_amount);
    return 0;

}

int create_all_threads(multi_lookup_struct *t, int amount_of_req_threads, int amount_of_res_threads) {
    
    if (amount_of_req_threads > MAX_REQUESTER_THREADS || amount_of_req_threads < 1) {
        fprintf(stderr, "%s", "Error: The number of requestor threads should be between 1 and 10.\n");
        return 1;
    }

    if (amount_of_res_threads > MAX_RESOLVER_THREADS || amount_of_res_threads < 1) {
        fprintf(stderr, "%s", "Error: The number of resolver threads should be between 1 and 10.\n");
        return 1;
    }

    for(int i = 0; i < amount_of_req_threads; i = i+1){
        pthread_create(&t->requester_threads_array[i], NULL, request_hostnames_from_files, t);
    }
    
    for(int i = 0; i < amount_of_res_threads; i = i+1){
        pthread_create(&t->resolver_threads_array[i], NULL, reslove_hostnames_from_files, t);
    }


    for(int i = 0; i < amount_of_req_threads; i = i+1){
        pthread_join(t->requester_threads_array[i], NULL);
    }
    
    t->requestor_threads_done = 1;  //requestor threads are done.
    fclose(t->file_pointer_for_requester_file);

    for(int i = 0; i < amount_of_res_threads; i = i+1){
        pthread_join(t->resolver_threads_array[i], NULL);
    }
    fclose(t->file_pointer_for_resolver_file);

    return 0;
}


void multi_lookup_free(multi_lookup_struct *t){
    array_free(&t->hostname_array);
    sem_destroy(&t->lock_requestor_file);
    sem_destroy(&t->lock_resolver_file);
    sem_destroy(&t->lock_accesing_input_files);
    sem_destroy(&t->lock_incrementing_array_put_amount);
    sem_destroy(&t->lock_incrementing_array_get_amount);

}

//gcc -ggdb multi-lookup.c -o multi-lookup.out -lpthread
//gcc -ggdb multi-lookup.c array.c -o multi-lookup.out -lpthread
//gdb -args multi-lookup.out 1 2 service.txt resolver.txt input.txt
//gcc -ggdb multi-lookup.c array.c util.c -o multi-lookup.out -lpthread

