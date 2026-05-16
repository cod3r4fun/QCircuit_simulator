/**
 * @file qmath.c
 * @brief Implementation of mathematical operations for complex numbers, matrices, and vectors.
 */


#include "types.h"
#include <stdlib.h>
#include <stdio.h>





long int power(long int base, int exponent){
    long int result = 1;
    for (int i=0; i<exponent; i++){
        result*= base;
    }
    return result;
}




Complex_number complex_add(Complex_number a, Complex_number b){
    return (Complex_number) {a.real+b.real, a.imag+b.imag};
}



Complex_number complex_multiply(Complex_number a, Complex_number b){
    return (Complex_number) {a.real*b.real - a.imag*b.imag, a.real*b.imag + a.imag*b.real};
}






Matrix *matrix_multiplication(const Matrix *A, const Matrix *B){

    if ((A == NULL) || (B== NULL)){
        fprintf(stderr, "Matrixes must be non null references for matrix multiplication");
        return NULL;
    }
    
    int rows_a = matrix_get_rows(A);
    int rows_b = matrix_get_rows(B);
    int cols_a = matrix_get_cols(A);
    int cols_b = matrix_get_cols(B);

    if (cols_a!=rows_b){
        fprintf(stderr, "Matrix multiplication requires the number of cols of the first Matrix A to be equal to the number of rows of the second Matrix B; The current number of cols is %d, whilst of rows is %d\n", cols_a, cols_b);
        return NULL;
    }

    Matrix *result = matrix_create(NULL, rows_a, cols_b);

    if (result == NULL){
        fprintf(stderr, "matrix creation failed in matrix multiplication");
        return NULL;
    }

    for (int i=0; i< rows_a; i++){
        for (int j=0; j<cols_b; j++){
            Complex_number temp = (Complex_number){0.0, 0.0};
            for (int z=0; z< cols_a; z++){
                temp = complex_add(temp, complex_multiply(matrix_get_element(A, i, z), matrix_get_element(B, z, j)));
            }
            matrix_set_element(result, temp, i, j);
        }
    }

    return result;
}




Vector *matrix_vector_multiplication(const Matrix *m, const Vector *v){
    return (Vector*) matrix_multiplication(m, v);
}