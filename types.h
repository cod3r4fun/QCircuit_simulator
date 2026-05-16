/**
 * @file types.h
 * @brief Data structures and function prototypes for complex matrix and vector operations.
 * * This header defines the public interface for creating, managing, and manipulating
 * matrices and vectors containing complex numbers. It uses opaque pointers to encapsulate
 * the internal memory layout.
 */

#ifndef TYPES_H
#define TYPES_H


//Complex number and general types section


/**
 * @brief Represents a complex number with real and imaginary components.
 */
typedef struct 
{
    double real; /**< The real part of the complex number */
    double imag; /**< The imaginary part of the complex number */
} Complex_number;



//Matrix section



/**
 * @brief Opaque type representing a 2D Matrix of complex numbers.
 */
typedef struct Matrix Matrix;

/**
 * @brief Allocates and initializes a new Matrix.
 * * @param data A pointer to an array of Complex_number to initialize the matrix with. 
 * If NULL, the matrix is initialized to all zeros.
 * @param rows The number of rows (must be > 0).
 * @param cols The number of columns (must be > 0).
 * @return A pointer to the newly created Matrix, or NULL if memory allocation fails 
 * or invalid dimensions are provided.
 */
Matrix* matrix_create(const Complex_number* data, int rows, int cols);

/**
 * @brief Frees the memory associated with a Matrix.
 * * @param m Pointer to the Matrix to be destroyed. If m is NULL, no operation is performed.
 */
void matrix_destroy(Matrix *m);

/**
 * @brief Retrieves an element from the Matrix at a specific row and column.
 * * @param m Pointer to the Matrix.
 * @param row The zero-indexed row position.
 * @param col The zero-indexed column position.
 * @return The Complex_number at the specified position. If the indices are out of bounds 
 * or the matrix is NULL, returns {DBL_MIN, DBL_MIN}.
 */
Complex_number matrix_get_element(const Matrix* m, int row, int col);

/**
 * @brief Sets the value of an element in the Matrix at a specific row and column.
 * * @param m Pointer to the Matrix.
 * @param value The Complex_number to insert into the Matrix.
 * @param row The zero-indexed row position.
 * @param col The zero-indexed column position.
 * @return 1 if the operation ended succesfully, -1 otherwise;
 */
int matrix_set_element(Matrix* m, Complex_number value, int row, int col);

/**
 * @brief Retrieves the number of rows in the Matrix.
 * * @param m Pointer to the Matrix.
 * @return The number of rows, or 0 if the matrix is NULL.
 */
int matrix_get_rows(const Matrix* m);

/**
 * @brief Retrieves the number of columns in the Matrix.
 * * @param m Pointer to the Matrix.
 * @return The number of columns, or 0 if the matrix is NULL.
 */
int matrix_get_cols(const Matrix* m);





//Vector section



/**
 * @brief A Vector is treated structurally as a 1D Matrix (N rows, 1 column).
 */
typedef struct Matrix Vector;

/**
 * @brief Allocates and initializes a new Vector.
 * * @param data A pointer to an array of Complex_number to initialize the vector with.
 * If NULL, the vector is initialized to all zeros.
 * @param length The total number of elements in the vector (must be > 0).
 * @return A pointer to the newly created Vector, or NULL if allocation fails.
 */
Vector* vector_create(const Complex_number* data, int length);

/**
 * @brief Frees the memory associated with a Vector.
 * * @param v Pointer to the Vector to be destroyed.
 */
void vector_destroy(Vector *v);

/**
 * @brief Retrieves an element from the Vector at a specific position.
 * * @param v Pointer to the Vector.
 * @param position The zero-indexed position in the vector.
 * @return The Complex_number at the specified position if it ends succesfully, or {DBL_MIN, DBL_MIN} otherwise;
 */
Complex_number vector_get_element(const Vector* v, int position);

/**
 * @brief Sets the value of an element in the Vector at a specific position.
 * * @param v Pointer to the Vector.
 * @param value The Complex_number to insert.
 * @param position The zero-indexed position in the vector.
 * @return 1 if the operation ended succesfully, -1 otherwise;
 */
int vector_set_element(Vector* v, Complex_number value, int position);

/**
 * @brief Retrieves the total length (number of elements) of the Vector.
 * * @param v Pointer to the Vector.
 * @return The length of the vector, or 0 if the vector is NULL.
 */
int vector_get_length(const Vector* v);






//Threading section


/**
 * @brief Contains the context needed for a single thread to compute (a segment of) matrix multiplication.
 */
typedef struct {
    Matrix *matrix1;   /**< Pointer to the first input matrix */
    Matrix *matrix2;   /**< Pointer to the second input matrix */
    Matrix *result;    /**< Pointer to the output matrix where results will be stored */
    //int start_row;     /**< The inclusive starting row index for this thread's computation workload */
    //int end_row;       /**< The exclusive ending row index for this thread's computation workload */
} ThreadMultTask;


/**
 * @brief Contains the context needed for a single thread to compute a partial misuration;
 */
typedef struct {
    double *probabilities;      /**< Pointer to the prob distribution array */
    int lenVector;      /**< length of the vector */
    int misurations;    /**< Number of misurations to do */
    int *results;       /**< Pointer to the frequentist occurrences array */
} ThreadMisTask;



#endif