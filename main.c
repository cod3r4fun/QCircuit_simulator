#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdint.h>
#include "types.h"
#include "qmath.h"
#include "parser.h"
#include "engine.h"

int main(int argc, char* argv[]){
    
    int opt;
    int thread_number = 1;
    char *state_file_path = NULL;
    char *circuit_file_path = NULL;

    while ((opt = getopt(argc, argv, "s:c:t:h")) != -1){
        switch (opt){
            case 's':
                state_file_path = optarg;
                break;
            
            case 'c':
                circuit_file_path = optarg;
                break;

            case 't':
                if (optarg){
                     thread_number = atoi(optarg);
                }
                break;

            case 'h':
                printf("Help: usage is:\n-s [specify target filename for qbit state description]\n-c [specify target filename for qbit circuit description]\n-t [specify desired number of threads; if not specified, maximum viable number will be selcted (half of the logical gates of the circuit)]");
                break;

            case '?':
                printf("Option invalid: for help see -h");
                break;
        }
    }

    if (state_file_path == NULL){
        printf("Error! State file should be specified with the -s option\n");
        return 1;
    }

    if (circuit_file_path == NULL){
        printf("Error! State file should be specified with the -c option\n");
        return 1;
    }

    struct stat st_state;
    struct stat st_circuit;

    if (stat(state_file_path, &st_state) == -1){
        perror("Error opening state file\n");
        return 1;
    }

    if (stat(circuit_file_path, &st_circuit) == -1){
        perror("Error opening circuit file\n");
        return 1;
    }

    char *state_data = malloc(st_state.st_size + 1);
    char *circuit_data = malloc(st_circuit.st_size + 1);

    FILE *state_file = fopen(state_file_path, "rb"); // Notice the "rb"
    if (state_file == NULL) {
        perror("Error opening state file");
        return 1;
    }
    
    size_t state_bytes_read = fread(state_data, 1, st_state.st_size, state_file);
    if (ferror(state_file)){
        perror("Error reading the state file, likely due to I/O interruption");
        fclose(state_file);
        return 1;
    }
    state_data[state_bytes_read] = '\0'; // Terminate using actual bytes read
    fclose(state_file);

    FILE *circuit_file = fopen(circuit_file_path, "rb"); // Notice the "rb"
    if (circuit_file == NULL) {
        perror("Error opening circuit file");
        return 1;
    }

    size_t circuit_bytes_read = fread(circuit_data, 1, st_circuit.st_size, circuit_file);
    if (ferror(circuit_file)){
        perror("Error reading the circuit file, likely due to I/O interruption");
        fclose(circuit_file);
        return 1;
    }
    circuit_data[circuit_bytes_read] = '\0'; // Terminate using actual bytes read
    fclose(circuit_file);

    remove_all_whitespace(state_data);
    remove_all_whitespace(circuit_data);

    int qbit_number = 0;
    int number_of_defined_gates = 0;
    int circuit_gate_number = 0;
    int measures = 0;
    Vector *initial_state = NULL;
    Vector *final_state = NULL;
    Matrix **circuit = NULL;
    Matrix *equivalent_final_matrix = NULL;

    if ((qbit_number = parse_inital_qbit_number(state_data)) == -1){
        perror("Error in file formatting, number of qbits could not be found\n");
        return 1;
    }

    if ((initial_state = parse_initial_state(state_data, qbit_number)) == NULL){
        perror("Error parsing the initial state; Please check file formatting or that there is sufficient memory\n");
        return 1;
    }

    if ((number_of_defined_gates = count_gates(circuit_data)) == 0){
        perror("Error, no gates could be detected. Please check file formatting\n");
        return 1;
    }

    if ((circuit = parse_circuit(circuit_data, &measures, &circuit_gate_number, qbit_number, number_of_defined_gates)) == NULL){
        perror("Error parsing the circuit; Please check file formatting or that there is sufficient memory\n");
        return 1;
    }

    if ((equivalent_final_matrix = mult_engine(circuit, circuit_gate_number, thread_number)) == NULL){
        perror("Error in multithreading operation, final Matrix could not be calculated. Please, try again.\n");
        return 1;
    }

    for (int i=0; i <circuit_gate_number; i++){
        if (circuit[i] != equivalent_final_matrix) {
        matrix_destroy(circuit[i]);
    }
    }
    free(circuit);

    if ((final_state = matrix_vector_multiplication(equivalent_final_matrix, initial_state)) == NULL){
        perror("Final multiplication failed. Please check memory availability");
        return 1;
    }

    free(initial_state);

    if (!measures){
        print_final_state(final_state);
        return 0;
    } else {
        int state_length = vector_get_length(final_state);
        double *probabilities = malloc(state_length * sizeof(double));
        //Debug
        //print_final_state(final_state);
        //
        for (int i = 0; i < state_length; i++) {
            // P(j) = |α_j|² = a² + b²
            double a = vector_get_element(final_state, i).real;
            double b = vector_get_element(final_state, i).imag;
            probabilities[i] = (a * a + b * b);
            //Debug
            //printf("Indice %d: Ampiezza (%.6f + i%.6f) --> Prob calcolata = %.6f\n", i, a, b, probabilities[i]);
            //
        }

        free(final_state);

        int *final_res = meas_engine(probabilities, state_length, thread_number, measures);
        if (final_res == NULL){
            perror("Error: multithreading measurement failed\n");
            return 1;
        }

        if (write_probability_distribution("output.txt", final_res, state_length, qbit_number, measures) == -1){
            perror("Error: final file could not be opened\n");
            free(final_res);
            return -1;
        }
        free(final_res);
        return 0;
    }
}