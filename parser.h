/**
 * @file parser.h
 * @brief Parsing utilities for reading and sanitizing quantum circuit definitions.
 *
 * This header defines the public interface for parsing quantum bit counts, 
 * initial state vectors, and complex circuit matrices from formatted text data.
 */

#ifndef PARSER_H
#define PARSER_H

#include "types.h"

/**
 * @brief Removes all whitespace characters (spaces, tabs, newlines) from a string in-place.
 *
 * This function modifies the original string by shifting characters leftward
 * to overwrite any whitespaces. It is highly recommended to run this on the 
 * raw file data before passing it to any of the parsing functions.
 *
 * @param str A pointer to the null-terminated string to be sanitized.
 */
void remove_all_whitespace(char *str);

/**
 * @brief Extracts the number of qubits defined in the input data.
 *
 * Searches the string for the specific qubit header (e.g., "#qbits") and 
 * parses the integer immediately following it.
 *
 * @param data The sanitized, null-terminated string containing the file data.
 * @return The integer number of qubits, or -1 if the header could not be found.
 */
int parse_inital_qbit_number(const char *data);

/**
 * @brief Parses the initial quantum state vector.
 *
 * Scans the data for the state initialization header (e.g., "#init[") and 
 * extracts the sequence of complex numbers to build the initial state vector.
 *
 * @param data The sanitized, null-terminated string containing the file data.
 * @param q_numb The number of qubits, used to calculate the required vector length (2^q_numb).
 * * @return A pointer to a newly allocated Vector containing the initial state, 
 * or NULL if the header is missing, memory allocation fails, or the 
 * parsed element count does not match the expected length.
 * * @note The caller is responsible for freeing the returned Vector using vector_destroy().
 */
Vector *parse_initial_state(const char *data, int q_numb);



/**
 * @brief Counts the number of gates present in the circuit
 * 
 * Scans the data for the gate initialization header (e.g., "#define") and extracts the number of gates present
 * @param data The sanitized, null-terminated string containing the file data.
 * * @return the number of gates which make up the circuit
 */
int count_gates(char *data);


/**
 * @brief Extracts the sequence of matrices which make up ithe circuit.
 *
 * @param data Sanitized string.
 * @param out_measure_repeats Output: misuration number (0 if absent).
 * @param out_num_gates Output: Number of logical gates which make up the circuit.
 * @param q_numb Qbit Number.
 * @param defined_gates Different logical gates defined number.
 * @return Matrix** pointer Array to Matrix, NULL in case of an error.
 */
Matrix **parse_circuit(const char *data, int *out_measure_repeats, int *out_num_gates, int q_numb, int defined_gates);


/**
 * @brief Prints the final quantum state vector to standard output.
 *
 * If no measurement is specified in the circuit, this function outputs the 
 * final state vector v_fin[cite: 16, 63]. The output follows the same format 
 * as the initial state: #init [val0,val1,...,valN]. 
 * Complex numbers are formatted with a maximum of six decimal places.
 *
 * @param v A pointer to the Vector representing the final state of the qubits.
 * @note This function should only be called when the measurement repetition 
 * count is zero.
 */
void print_final_state(Vector *v);

/**
 * @brief Writes the estimated probability distribution to a file.
 *
 * For each possible state (0 to 2^n - 1), it calculates the frequency 
 * (counts / total) and prints the state in binary format followed by 
 * the probability (e.g., "01 @ 0.4").
 *
 * @param filepath The destination path for the output file.
 * @param counts Array of integers containing occurrences of each state.
 * @param length The size of the counts array (2^q_numb).
 * @param q_numb The number of qubits (determines binary string length).
 * @param total_measurements The total number of iterations performed.
 * @return 0 on success, -1 if the file could not be opened.
 */
int write_probability_distribution(const char *filepath, int *counts, int length, int q_numb, int total_measurements);

#endif