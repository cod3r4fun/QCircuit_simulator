#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include "types.h"
#include "qmath.h"


pthread_mutex_t my_lock;




void* worker_thread_mult(void* arg) {
    // 1. Unpack the void pointer back into mult struct
    ThreadMultTask* task = (ThreadMultTask*)arg;
    
    // 2. Do the heavy lifting (calling your math module)
    task->result = matrix_multiplication(task->matrix1, task->matrix2);
    
    // 3. Exit safely
    pthread_exit(NULL);
}




Matrix *mult_engine(Matrix **circuit, int numberOfGates, int numberOfThreads){
    if (numberOfThreads==0){
        fprintf(stderr, "Cannot execute with 0 threads");
        return NULL;
    }

    int NGates = numberOfGates;

    ThreadMultTask *threadOrder = malloc(numberOfGates * sizeof(ThreadMultTask));
    int *is_intermediate = calloc(numberOfGates, sizeof(int));
    pthread_t *tidArr = malloc(numberOfGates * sizeof(pthread_t));

    for (int i=0; i<numberOfGates; i++){
        threadOrder[i].result = circuit[i];
        is_intermediate[i] = 0;
    }


    
    // poichè tutte le matrice hanno fondalmentalmente la stessa dimensione, utilizzare un modello ad albero non porta a degradazioni di efficienza significativi, 
    // con un vantaggio non trascurabile in leggibilità del codice

    while (NGates > 1){
        int NTemp = NGates - (NGates & 1);

        /*
        // Sufficienti thread (non si devono riutilzzare)
        if (numberOfThreads >= NTemp/2){

            
            
            for (int i=0; i<NTemp/2; i++){
                threadOrder[i].matrix1 = threadOrder[2*i].result;
                threadOrder[i].matrix2 = threadOrder[2*i+1].result;
                pthread_create(&tidArr[i], 0, worker_thread_func, &threadOrder[i]);
            }
            
            for (int i=0; i<NTemp/2; i++){
                pthread_join(tidArr[i], NULL);
                if (is_intermediate[2*i]) {
                    free(threadOrder[i].matrix1); 
                }
                if (is_intermediate[2*i+1]) {
                    free(threadOrder[i].matrix2);
                }
                
                is_intermediate[i] = 1;
                }
            
            // Numero gate dispari
            if (NGates & 1) {
                threadOrder[NTemp/2].result = threadOrder[NTemp].result;
                is_intermediate[NTemp/2] = is_intermediate[NTemp];
            }

        }

        // Insufficieni thread (si devono riutilizzare)
        else {





        ------------> Si può notare il seguente modello in realà funziona per ambedue i casi <--------------------------------

        */



            int i=0;
            while (i<NTemp/2){
                int j = i + numberOfThreads;
                if (j > NTemp/2) j = NTemp/2;
                for (int k=i; k<j; k++){
                    threadOrder[k].matrix1 = threadOrder[2*k+1].result;
                    threadOrder[k].matrix2 = threadOrder[2*k].result;
                    pthread_create(&tidArr[k], 0, worker_thread_mult, &threadOrder[k]);
                }
                for (int k = i; k < j; k++){
                pthread_join(tidArr[k], NULL);
                if (is_intermediate[2*k+1]) {
                    matrix_destroy(threadOrder[k].matrix1); 
                }
                if (is_intermediate[2*k]) {
                    matrix_destroy(threadOrder[k].matrix2);
                }
                
                is_intermediate[k] = 1;
                }
                i = j;
            }
            

            // Numero gate dispari
            if (NGates & 1) {
                threadOrder[NTemp/2].result = threadOrder[NTemp].result;
                is_intermediate[NTemp/2] = is_intermediate[NTemp];
            }

        NGates = NTemp/2 + (NGates & 1);
    }

    Matrix *final_result = threadOrder[0].result;
    free(threadOrder);
    free(is_intermediate);
    free(tidArr);


    return final_result;
}





/**
 * Worker thread per la misurazione
 */
void *worker_thread_mis(void* arg) {
    ThreadMisTask* task = (ThreadMisTask*) arg;

    // 1. Setup GSL Random Number Generator for this thread
    //Thread ID or a combination of time and address to seed uniquely
    gsl_rng *r = gsl_rng_alloc(gsl_rng_default);
    unsigned long seed = (unsigned long)pthread_self(); 
    gsl_rng_set(r, seed);
    
    // 2. Pre-process the distribution for efficient sampling
    gsl_ran_discrete_t *gsl_dist = gsl_ran_discrete_preproc(task->lenVector, task ->probabilities);

    // 3. Local results buffer to minimize mutex locking
    int *local_occurences = calloc(task->lenVector, sizeof(int));

    // 4. Sampling loop
    for (int m = 0; m < task->misurations; m++) {
        size_t sample = gsl_ran_discrete(r, gsl_dist);
        local_occurences[sample]++;
    }

    // 5. Synchronize results with the shared global array
    pthread_mutex_lock(&my_lock);
    for (int i = 0; i < task->lenVector; i++) {
        task->results[i] += local_occurences[i];
    }
    pthread_mutex_unlock(&my_lock);

    // 6. Cleanup
    gsl_ran_discrete_free(gsl_dist);
    gsl_rng_free(r);
    free(local_occurences);

    return NULL;
}


int *meas_engine(double *probabilities, int length, int number_of_threads, int number_of_measurements){
    if (number_of_measurements < number_of_threads) number_of_threads = number_of_measurements;

    int op_per_thread_base = number_of_measurements / number_of_threads;
    int op_rimanenti = number_of_measurements % number_of_threads;
    int *result = calloc(length, sizeof(int));
    if (result == NULL){
        perror("Memory allocation for final result vector failed");
        return NULL;
    }

    pthread_t tid[number_of_threads];
    ThreadMisTask thread_task[number_of_threads];

    for (int i=0; i<number_of_threads; i++){
        thread_task[i].probabilities = probabilities;
        thread_task[i].lenVector = length;
        if (op_rimanenti){
            thread_task[i].misurations = op_per_thread_base+1;
            op_rimanenti--;
        } else{
            thread_task[i].misurations = op_per_thread_base;
        }
        thread_task[i].results = result;
    }

    pthread_mutex_init(&my_lock, NULL);

    for (int i=0; i<number_of_threads; i++){
        
        pthread_create(&tid[i], 0, worker_thread_mis, &thread_task[i]);
    }

    for (int i=0; i<number_of_threads; i++){
        pthread_join(tid[i], NULL);
    }
    pthread_mutex_destroy(&my_lock);

    return result;
}