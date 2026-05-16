#include "types.h"

#ifndef QMATH_H
#define QMATH_H


/**
 * @file qmath.h
 * @brief Mathematical operations for complex numbers, matrices, and vectors.
 */



/**
 * @brief Calcola l'elevamento a potenza di un intero.
 *
 * @param base     Il valore intero (long) da elevare.
 * @param exponent L'esponente intero (positivo) a cui elevare la base.
 * 
 * @return long    Il risultato di base^exponent. 
 *                 Ritorna 1 se l'esponente è 0.
 * 
 * @note           Attenzione: Non gestisce esponenti negativi. 
 *                 Per esponenti grandi, il risultato potrebbe eccedere 
 *                 il limite massimo di 'long' (Integer Overflow).
 */

long int power(long int base, int exponent);




/**
 * @brief Adds two complex numbers.
 * * @param a The first complex number.
 * @param b The second complex number.
 * @return The sum of a and b.
 */
Complex_number complex_add(Complex_number a, Complex_number b);




/**
 * @brief Multiplies two complex numbers.
 * * @param a The first complex number.
 * @param b The second complex number.
 * @return The product of a and b.
 */
Complex_number complex_multiply(Complex_number a, Complex_number b);




/**
 * @brief Multiplies two matrices containing complex numbers.
 * * Performs standard matrix multiplication (A * B). The number of columns in 
 * Matrix A must equal the number of rows in Matrix B.
 * * @param A Pointer to the first matrix.
 * @param B Pointer to the second matrix.
 * @return A pointer to a newly allocated resulting Matrix, or NULL if the 
 * dimensions are incompatible or pointers are NULL.
 */
Matrix *matrix_multiplication(const Matrix *A, const Matrix *B);



/**
 * @brief Multiplies a matrix by a vector.
 * * Computes M * v. Because 'v' is structured as a column vector (N x 1),
 * this operation natively utilizes standard matrix multiplication.
 * * @param m Pointer to the Matrix.
 * @param v Pointer to the Vector.
 * @return A pointer to a newly allocated resulting Vector, or NULL if dimensions 
 * are incompatible or pointers are NULL.
 */
Vector *matrix_vector_multiplication(const Matrix *m, const Vector *v);







#endif