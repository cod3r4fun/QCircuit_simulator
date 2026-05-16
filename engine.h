#ifndef ENGINE_H
#define ENGINE_H

#include "types.h"

/**
 * @file engine.h
 * @brief Core execution engine for quantum circuit matrix reduction.
 * * This module handles the parallelized multiplication of gate matrices 
 * using a multi-threaded binary tree reduction approach.
 */



/**
 * @brief Reduces a sequence of matrices into a single final state matrix.
 * * This engine takes an array of gates and multiplies them in pairs using a 
 * binary tree reduction. This approach maximizes thread utilization and 
 * maintains O(log N) depth.
 * * @note This function manages intermediate memory; it will free intermediate 
 * matrices created during the reduction process.
 * * @param circuit An array of pointers to Matrix structures representing the gates.
 * @param numberOfGates The total number of matrices in the circuit array.
 * @param numberOfThreads The maximum number of concurrent threads to spawn.
 * @return Matrix* A pointer to the resulting combined Matrix, or NULL on error.
 */
Matrix *mult_engine(Matrix **circuit, int numberOfGates, int numberOfThreads);


/**
 * @brief Executes the measurement phase using multiple threads.
 *
 * This function calculates the probability distribution induced by the final 
 * probability distribution. It distributes the total number of measurements 
 * across the specified number of threads.
 *
 * Each measurement produces a state index j with probability |alpha_j|^2.
 *
 * @param probabilities The final probability distribution
 * @param length The dimension of the vector (2^n) from which the probability distribution was obtained.
 * @param number_of_threads Number of threads to use for computation.
 * @param number_of_measurements Total number of samples to take.
 * @return int* An array of integers of size 'length' containing the count 
 * of each state, or NULL on failure.
 * @note The caller is responsible for freeing the returned array.
 */
int *meas_engine(double *probabilities, int length, int number_of_threads, int number_of_measurements);

#endif