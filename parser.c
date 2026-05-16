#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "qmath.h"




// HELPER FUNCTIONS 
 

static int custom_strncmp(const char *s1, const char *s2, int n) {
    for (int i = 0; i < n; i++) {
        if (s1[i] != s2[i]) return s1[i] - s2[i];
        if (s1[i] == '\0') return 0;
    }
    return 0;
}

static int custom_strlen(const char *s) {
    int i = 0;
    while (s[i] != '\0') i++;
    return i;
}

// Struttura temporanea per mappare il nome di un gate alla sua matrice
typedef struct {
    char name[32];
    Matrix *matrix;
} GateDef;




// PARSER NUMERI COMPLESSI

// Legge un numero complesso nel formato specificato dal PDF (es. 0.77+i0.20 oppure 1.0)
static Complex_number parse_complex(const char **ptr) {
    Complex_number c = {0.0, 0.0};
    char *end_ptr;

    // 1. Handle pure imaginary starting directly with 'i' (e.g., "i")
    if (**ptr == 'i') {
        c.real = 0.0;
        (*ptr)++; // Move pointer past the 'i'
        
        // If there is a number after 'i' (like "i0.5"), read it. Otherwise, it is 1.0.
        c.imag = strtod(*ptr, &end_ptr);
        if (*ptr == end_ptr) {
            c.imag = 1.0; 
        } else {
            *ptr = end_ptr;
        }
        return c;
    }

    // 2. Read the first number (could be the real part, or nothing if it starts with '-i')
    c.real = strtod(*ptr, &end_ptr);
    
    // If strtod successfully read a number, advance the pointer
    if (*ptr != end_ptr) {
        *ptr = end_ptr;
    }

    // 3. Check for imaginary part starting with '+' or '-'
    if (**ptr == '+' || **ptr == '-') {
        char sign = **ptr;
        
        // Check if it is followed by 'i' (e.g., "+i", "-i")
        if (*(*ptr + 1) == 'i') {
            (*ptr) += 2; // Skip the sign and the 'i'
            
            // Read the number after 'i' (e.g., "+i0.5")
            c.imag = strtod(*ptr, &end_ptr);
            
            // If there is no number after 'i', it implies a coefficient of 1.0
            if (*ptr == end_ptr) {
                c.imag = 1.0;
            } else {
                *ptr = end_ptr;
            }
            
            // Apply the saved sign
            if (sign == '-') {
                c.imag = -c.imag;
            }
        }
    }

    return c;
}






// PUBLIC FUNCTIONS


    void remove_all_whitespace(char *str) {
    char *read = str, *write = str;
    while (*read) {
        if (!isspace((unsigned char)*read)) {
            *write++ = *read;
        }
        read++;
    }
    *write = '\0';
}


// data non deve contenere whitespaces per nessuno dei metodi a seguire


// PARSER FILE 1: QBITS & INITIAL STATE

int parse_inital_qbit_number(const char *data) {
    const char *ptr = data;
    
    // Scorre la stringa finché non trova "#qbits"
    while (*ptr != '\0') {
        if (custom_strncmp(ptr, "#qubits", 7) == 0) {
            ptr += 7; // Salta l'header
            char *temp = NULL;
            return (int) strtol(ptr, &temp, 10);
        }
        ptr++;
    }

    fprintf(stderr, "No number of qbits could be found during parsing.\n");
    return -1;
}

Vector *parse_initial_state(const char *data, int q_numb) {
    const char *ptr = data;
    int header_found = 0;

    // 1. Cerca l'header "#init["
    while (*ptr != '\0') {
        if (custom_strncmp(ptr, "#init[", 6) == 0) {
            ptr += 6; // Punta direttamente al primo numero dopo '['
            header_found = 1;
            break;
        }
        ptr++;
    }

    if (!header_found) {
        fprintf(stderr, "Could not find the state header '#init['.\n");
        return NULL;
    }

    // 2. Crea il vettore vuoto
    int vector_length = (int) power(2, (long int) q_numb);
    Vector *result = vector_create(NULL, vector_length);
    if (result == NULL) return NULL;

    int vector_pos = 0;

    // 3. Estrae i numeri complessi usando la helper function
    while (*ptr != ']' && *ptr != '\0' && vector_pos < vector_length) {
        
        Complex_number val = parse_complex(&ptr);
        vector_set_element(result, val, vector_pos);
        vector_pos++;

        // Salta la virgola che separa gli elementi del vettore (se presente)
        if (*ptr == ',') {
            ptr++;
        }
    }

    // 4. Controllo di integrità
    if (vector_pos != vector_length) {
        fprintf(stderr, "Something went wrong with parsing the vector. Expected: %d, parsed: %d\n", vector_length, vector_pos);
        vector_destroy(result);
        return NULL;
    }

    return result;
}

 
 



 //Contatore numero di porte

 int count_gates(char *data){
    int total_defs = 0;
    const char *scan_ptr = data;
    while (*scan_ptr != '\0') {
        if (custom_strncmp(scan_ptr, "#define", 7) == 0) total_defs++;
        scan_ptr++;
    }

    return total_defs;
 }


// PARSER CIRCUITO PRINCIPALE

Matrix **parse_circuit(const char *data, int *out_measure_repeats, int *out_num_gates, int q_numb, int total_defs) {
    if (out_measure_repeats == NULL) *out_measure_repeats = 0;
    if (out_num_gates == NULL) *out_num_gates = 0;

    // FASE 1: allocazione dizionario temporaneo (#define)

    GateDef *dict = NULL;
    if (total_defs > 0) {
        dict = malloc(total_defs * sizeof(GateDef));
    }
    
    int num_defs = 0;
    const char *ptr = data;
    int dim = power(2, q_numb);

    // Riempimento dizionario
    while (*ptr != '\0') {
        if (custom_strncmp(ptr, "#define", 7) == 0) {
            ptr += 7;
            int name_len = 0;
            while (ptr[name_len] != '[' && ptr[name_len] != '\0' && name_len < 31) {
                dict[num_defs].name[name_len] = ptr[name_len];
                name_len++;
            }
            dict[num_defs].name[name_len] = '\0';
            ptr += name_len + 1; // Salta nome e '['

            Matrix *m = matrix_create(NULL, dim, dim);
            for (int r = 0; r < dim; r++) {
                if (*ptr == '(') ptr++;
                for (int c = 0; c < dim; c++) {
                    if (*ptr == ']' || *ptr == '\0') {
                        fprintf(stderr, "Error: Matrix '%s' is underdefined. Expected %d elements (for %d qubits), but got fewer.\n", dict[num_defs].name, dim * dim, q_numb);
                        matrix_destroy(m);
                        // Clean up the dictionary and abort safely
                        for (int k = 0; k < num_defs; k++) matrix_destroy(dict[k].matrix);
                        free(dict);
                        return NULL;
                    }
                    matrix_set_element(m, parse_complex(&ptr), r, c);
                    if (*ptr == ',') ptr++;
                }
                if (*ptr == ')') ptr++;
            }
            if (*ptr != ']') {
                fprintf(stderr, "Error: Matrix '%s' is overdefined. Expected exactly %d elements (for %d qubits).\n", dict[num_defs].name, dim * dim, q_numb);
                matrix_destroy(m);
                // Clean up the dictionary and abort safely
                for (int k = 0; k < num_defs; k++) matrix_destroy(dict[k].matrix);
                free(dict);
                return NULL;
            }

            if (*ptr == ']') ptr++;
            dict[num_defs++].matrix = m;
        } else if (custom_strncmp(ptr, "#circ", 5) == 0) break;
        else ptr++;
    }

    // FASE 2: Conteggio dei gate effettivamente usati nel circuito (#circ)
    const char *circ_ptr = data;
    while (*circ_ptr != '\0' && custom_strncmp(circ_ptr, "#circ", 5) != 0) circ_ptr++;
    if (*circ_ptr == '\0') {
        perror("Error: #circ non trovato");
        free(dict);
        return NULL; 
    }

    circ_ptr += 5;
    const char *temp_ptr = circ_ptr;
    int gate_count = 0;

    // Conteggio num token validi usando il "Longest Match"
    while (*temp_ptr != '\0' && custom_strncmp(temp_ptr, "measure", 7) != 0) {
        int best_match_idx = -1;
        int max_len = 0;
        
        for (int i = 0; i < num_defs; i++) {
            int nlen = custom_strlen(dict[i].name);
            if (custom_strncmp(temp_ptr, dict[i].name, nlen) == 0) {
                if (nlen > max_len) {
                    max_len = nlen;
                    best_match_idx = i;
                }
            }
        }
        
        if (best_match_idx != -1) {
            gate_count++;
            temp_ptr += max_len; // Salta il nome del gate trovato
        } else {
            temp_ptr++; // Ignora caratteri non riconosciuti
        }
    }

    // FASE 3: Allocazione dell'array di ritorno e mappatura
    Matrix **sequence = malloc(gate_count * sizeof(Matrix *));
    int seq_idx = 0;
    temp_ptr = circ_ptr;

    while (*temp_ptr != '\0' && custom_strncmp(temp_ptr, "measure", 7) != 0) {
        int best_match_idx = -1;
        int max_len = 0;
        
        for (int i = 0; i < num_defs; i++) {
            int nlen = custom_strlen(dict[i].name);
            if (custom_strncmp(temp_ptr, dict[i].name, nlen) == 0) {
                if (nlen > max_len) {
                    max_len = nlen;
                    best_match_idx = i;
                }
            }
        }

        if (best_match_idx != -1) {
            // Inseriamo la matrice nella sequenza (creando una copia per sicurezza)
            sequence[seq_idx++] = matrix_create(NULL, dim, dim);
            for(int r=0; r<dim; r++) {
                for(int c=0; c<dim; c++) {
                    matrix_set_element(sequence[seq_idx-1], matrix_get_element(dict[best_match_idx].matrix, r, c), r, c);
                }
            }
            temp_ptr += max_len;
        } else {
            temp_ptr++;
        }
    }

    // Gestione misura finale
    if (custom_strncmp(temp_ptr, "measure", 7) == 0) {
        temp_ptr += 7;
        char *end;
        if (out_measure_repeats) *out_measure_repeats = (int)strtol(temp_ptr, &end, 10);
    }

    // PULIZIA
    for (int i = 0; i < num_defs; i++) matrix_destroy(dict[i].matrix);
    if (dict) free(dict);

    if (out_num_gates) *out_num_gates = gate_count;
    return sequence;
}



void print_final_state(Vector *v) {
    if (v == NULL) {
        fprintf(stderr, "Error: Cannot print a NULL vector.\n");
        return;
    }

    printf("#init [");
    for (int i = 0; i < vector_get_length(v); i++) {
        Complex_number val = vector_get_element(v, i);

        // Print the real part with up to 6 decimal places 
        printf("%.6g", val.real);

        // Print the imaginary part only if it exists
        if (val.imag > 0) {
            printf("+i%.6g", val.imag);
        } else if (val.imag < 0) {
            // Using -val.imag because the '-' is handled by the literal string
            printf("-i%.6g", -val.imag);
        }

        // Add a comma between elements except for the last one
        if (i < vector_get_length(v) - 1) {
            printf(",");
        }
    }
    printf("]\n");
}



int write_probability_distribution(const char *filepath, int *counts, int length, int q_numb, int total_measurements) {
    FILE *f = fopen(filepath, "w");
    if (f == NULL) {
        perror("Error opening output file");
        return -1;
    }

    if (total_measurements <= 0) {
        fprintf(stderr, "Invalid number of measurements.\n");
        fclose(f);
        return -1;
    }

    for (int i = 0; i < length; i++) {
        // 1. Generate Binary String for the state
        // We iterate from the most significant bit to the least
        for (int b = q_numb - 1; b >= 0; b--) {
            int bit = (i >> b) & 1;
            fprintf(f, "%d", bit);
        }

        // 2. Calculate the frequentist probability [cite: 37, 38]
        double prob = (double)counts[i] / total_measurements;

        // 3. Print in the required format: binary @ probability
        // Using %.6g to maintain consistency with the precision requirements
        fprintf(f, " @ %.6g\n", prob);
    }

    fclose(f);
    return 0;
}