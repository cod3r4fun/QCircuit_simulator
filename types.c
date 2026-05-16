/**
 * @file types.c
 * @brief Implementation of the matrix and vector data structures and their operations.
 */

#include "types.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <float.h>

/**
 * @brief Internal representation of the Matrix structure.
 * * The 2D matrix data is flattened into a 1D contiguous array in memory
 * to improve cache locality and simplify allocation.
 */
struct Matrix
{
    int rows;             /**< Number of rows in the matrix */
    int cols;             /**< Number of columns in the matrix */
    Complex_number *data; /**< Flattened 1D array storing the complex numbers */
};



Matrix* matrix_create(const Complex_number* data, int rows, int cols){
    Matrix *m = calloc(1, sizeof(Matrix));
    if (m == NULL) return NULL;

    if (rows < 1) {
        fprintf(stderr, "The number of rows in a Matrix should always be positive and bigger than 0; current number: %d\n", rows);
        free(m);
        return NULL;
    }
    if (cols < 1) {
        fprintf(stderr, "The number of cols in a Matrix should always be positive and bigger than 0; current number: %d\n", cols);
        free(m);
        return NULL;
    }

    m->rows = rows;
    m->cols = cols;
    m->data = calloc(rows * cols, sizeof(Complex_number));
    
    if (m->data == NULL){
        free(m);
        return NULL;
    }

    if (data != NULL){
        memcpy(m->data, data, rows * cols * sizeof(Complex_number));
    }

    return m;
}

void matrix_destroy(Matrix *m){
    if (m != NULL){
        free(m->data);
        free(m);
    }
}

Complex_number matrix_get_element(const Matrix *m, int row, int col){
    if (m == NULL || row >= m->rows || col >= m->cols || row < 0 || col < 0) {
        return (Complex_number) {DBL_MIN, DBL_MIN};
    }

    return m->data[row * m->cols + col];
}

int matrix_set_element(Matrix* m, Complex_number value, int row, int col) {
    if (m == NULL || row >= m->rows || col >= m->cols || row < 0 || col < 0) {
        return -1;
    }
    m->data[row * m->cols + col] = value;
    return 1;
}


int matrix_get_rows(const Matrix* m) {
    if (m == NULL) {
        fprintf(stderr, "reference to null Matrix object\n");
        return 0;
    }
    return m->rows;
}

int matrix_get_cols(const Matrix* m) {
    if (m == NULL) {
        fprintf(stderr, "reference to null Matrix object\n");
        return 0;
    }
    return m->cols;
}




Vector* vector_create(const Complex_number* data, int size) {
    return matrix_create(data, size, 1);
}

void vector_destroy(Vector *v) {
    matrix_destroy(v);
}

Complex_number vector_get_element(const Vector* v, int position){
    return matrix_get_element(v, position, 0);
}

int vector_set_element(Vector* v, Complex_number value, int position){
    return matrix_set_element(v, value, position, 0);
}

int vector_get_length(const Vector* v) {
    if (v == NULL) {
        fprintf(stderr, "reference to null Vector object\n");
        return 0;
    }
    return matrix_get_rows(v); 
}