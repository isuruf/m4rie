/**
 * \file gf2e_matrix.h
 *
 * \brief Dense matrices over GF(2^k) (2<= k <= 10) represented by M4RI matrices.
 *
 * This file implements the data type mzed_t. 
 * That is, matrices over GF(2^k) in row major representation. 

 * For example, let a = \sum a_i x_i / <f> and b = \sum b_i x_i / <f> 
 * be elements in GF(2^6) with minimal polynomial f. Then, the 
 * 1 x 2 matrix [b a] would be stored as 
\verbatim
 [...| 0 0 b5 b4 b3 b2 b1 b0 | 0 0 a5 a4 a3 a2 a1 a0]
\endverbatim
 * 
 * Internally M4RI matrices are used to store bits with allows to
 * re-use existing M4RI methods (such as mzd_add) when implementing
 * functions for mzed_t.
 *
 * \author Martin Albrecht <martinralbrecht@googlemail.com>
 */

#ifndef M4RIE_GF2E_MATRIX_H
#define M4RIE_GF2E_MATRIX_H

/******************************************************************************
*
*            M4RIE: Linear Algebra over GF(2^e)
*
*    Copyright (C) 2010,2011 Martin Albrecht <martinralbrecht@googlemail.com>
*
*  Distributed under the terms of the GNU General Public License (GEL)
*  version 2 or higher.
*
*    This code is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*    General Public License for more details.
*
*  The full text of the GPL is available at:
*
*                  http://www.gnu.org/licenses/
******************************************************************************/

#include <m4ri/m4ri.h>
#include "finite_field.h"
#include "m4ri_functions.h"

/**
 * \brief Dense matrices over GF(2^k) in a packed representation.
 * 
 * \ingroup Definitions
 */

typedef struct {

  /**
   * m x n matrices over GF(2^k) are represented as m x (kn) matrices over GF(2).
   */
  mzd_t *x;

  /**
   * A finite field GF(2^k).
   */

  gf2e *finite_field;

  /**
   * Number of rows.
   */

  rci_t nrows;

  /**
   * Number of columns.
   */

  rci_t ncols;

  /**
   * The internal width of elements (must divide 64)
   */

  wi_t w;

} mzed_t;


/**
 * \brief Create a new matrix of dimension m x n over ff
 *
 * Use mzed_free to kill it.
 *
 * \param ff Finite field
 * \param m Number of rows
 * \param n Number of columns
 *
 * \ingroup Constructions
 */

mzed_t *mzed_init(gf2e *ff, const rci_t m, const rci_t n);

/**
 * \brief Free a matrix created with mzed_init.
 * 
 * \param A Matrix
 *
 * \ingroup Constructions
 */

void mzed_free(mzed_t *A);


/**
 * \brief Concatenate B to A and write the result to C.
 * 
 * That is,
 *
 \verbatim
 [ A ], [ B ] -> [ A  B ] = C
 \endverbatim
 *
 * The inputs are not modified but a new matrix is created.
 *
 * \param C Matrix, may be NULL for automatic creation
 * \param A Matrix
 * \param B Matrix
 *
 * \note This is sometimes called augment.
 *
 * \ingroup Constructions
 */

static inline mzed_t *mzed_concat(mzed_t *C, const mzed_t *A, const mzed_t *B) {
  if(C==NULL)
    C = mzed_init(A->finite_field, A->nrows, A->ncols + B->ncols);
  mzd_concat(C->x, A->x, B->x);
  return C;
}

/**
 * \brief Stack A on top of B and write the result to C.
 *
 * That is, 
 *
 \verbatim
 [ A ], [ B ] -> [ A ] = C
                 [ B ]
 \endverbatim
 *
 * The inputs are not modified but a new matrix is created.
 *
 * \param C Matrix, may be NULL for automatic creation
 * \param A Matrix
 * \param B Matrix
 *
 * \ingroup Constructions
 */

static inline mzed_t *mzed_stack(mzed_t *C, const mzed_t *A, const mzed_t *B) {
  if(C==NULL)
    C = mzed_init(A->finite_field, A->nrows + B->nrows, A->ncols);
  mzd_stack(C->x, A->x, B->x);
  return C;
}


/**
 * \brief Copy a submatrix.
 * 
 * Note that the upper bounds are not included.
 *
 * \param S Preallocated space for submatrix, may be NULL for automatic creation.
 * \param M Matrix
 * \param lowr start rows
 * \param lowc start column
 * \param highr stop row (this row is \em not included)
 * \param highc stop column (this column is \em not included)
 *
 * \ingroup Constructions
 */
static inline mzed_t *mzed_submatrix(mzed_t *S, const mzed_t *M, const rci_t lowr, const rci_t lowc, const rci_t highr, const rci_t highc) {
  if(S==NULL)
    S = mzed_init(M->finite_field, highr - lowr, highc - lowc);

  mzd_submatrix(S->x, M->x, lowr, lowc*M->w, highr, highc*M->w);
  return S;
}

/**
 * \brief Create a window/view into the matrix M.
 *
 * A matrix window for M is a meta structure on the matrix M. It is
 * setup to point into the matrix so M \em must \em not be freed while the
 * matrix window is used.
 *
 * This function puts the restriction on the provided parameters that
 * all parameters must be within range for M which is not currently
 * enforced.
 *
 * Use mzed_free_window to free the window.
 *
 * \param M Matrix
 * \param lowr Starting row (inclusive)
 * \param lowc Starting column (inclusive)
 * \param highr End row (exclusive)
 * \param highc End column (exclusive)
 *
 * \ingroup Constructions
 */

static inline mzed_t *mzed_init_window(const mzed_t *A, const rci_t lowr, const rci_t lowc, const rci_t highr, const rci_t highc) {
  mzed_t *B = (mzed_t *)m4ri_mm_malloc(sizeof(mzed_t));
  B->finite_field = A->finite_field;
  B->w = gf2e_degree_to_w(A->finite_field);
  B->nrows = highr - lowr;
  B->ncols = highc - lowc;
  B->x = mzd_init_window(A->x, lowr, B->w*lowc, highr, B->w*highc);
  return B;
}

/**
 * \brief Free a matrix window created with mzed_init_window.
 * 
 * \param A Matrix
 *
 * \ingroup Constructions
 */

static inline void mzed_free_window(mzed_t *A) {
  mzd_free_window(A->x);
  m4ri_mm_free(A);
}

/**
 * \brief Set C = A+B.
 *
 * C is also returned. If C is NULL then a new matrix is created which
 * must be freed by mzed_free.
 *
 * \param C Preallocated sum matrix, may be NULL for automatic creation.
 * \param A Matrix
 * \param B Matrix
 *
 * \ingroup Addition
 */

mzed_t *mzed_add(mzed_t *C, const mzed_t *A, const mzed_t *B);

/**
 * \brief Same as mzed_add but without any checks on the input.
 *
 * \param C Preallocated sum matrix, may be NULL for automatic creation.
 * \param A Matrix
 * \param B Matrix
 *
 * \wordoffset
 *
 * \ingroup Addition
 */

mzed_t *_mzed_add(mzed_t *C, const mzed_t *A, const mzed_t *B);

/**
 * \brief Same as mzed_add.
 *
 * \param C Preallocated difference matrix, may be NULL for automatic creation.
 * \param A Matrix
 * \param B Matrix
 *
 * \wordoffset
 *
 * \ingroup Addition
 */

#define mzed_sub mzed_add

/**
 * \brief Same as mzed_sub but without any checks on the input.
 *
 * \param C Preallocated difference matrix, may be NULL for automatic creation.
 * \param A Matrix
 * \param B Matrix
 *
 * \wordoffset
 *
 * \ingroup Addition
 */

#define _mzed_sub _mzed_add

/**
 * \brief Compute C such that C == AB.
 *
 * \param C Preallocated return matrix, may be NULL for automatic creation.
 * \param A Input matrix A.
 * \param B Input matrix B.
 *
 * \ingroup Multiplication
 */
 
mzed_t *mzed_mul(mzed_t *C, const mzed_t *A, const mzed_t *B);

/**
 * \brief Compute C such that C == C + AB.
 *
 * \param C Preallocated product matrix, may be NULL for automatic creation.
 * \param A Input matrix A.
 * \param B Input matrix B.
 *
 * \ingroup Multiplication
 */

mzed_t *mzed_addmul(mzed_t *C, const mzed_t *A, const mzed_t *B);

/**
 * \brief C such that C == AB.
 *
 * \param C Preallocated product matrix.
 * \param A Input matrix A.
 * \param B Input matrix B.
 *
 * \ingroup Multiplication
 */

mzed_t *_mzed_mul(mzed_t *C, const mzed_t *A, const mzed_t *B);

/**
 * \brief C such that C == C + AB.
 *
 * \param C Preallocated product matrix.
 * \param A Input matrix A.
 * \param B Input matrix B.
 *
 * \ingroup Multiplication
 */

mzed_t *_mzed_addmul(mzed_t *C, const mzed_t *A, const mzed_t *B);


/**
 * \brief Compute C such that C == C + AB using naive cubic multiplication.
 *
 * \param C Preallocated product matrix, may be NULL for automatic creation.
 * \param A Input matrix A.
 * \param B Input matrix B.
 *
 * \ingroup Multiplication
 */

mzed_t *mzed_addmul_naive(mzed_t *C, const mzed_t *A, const mzed_t *B);

/**
 * \brief Compute C such that C == AB using naive cubic multiplication.
 *
 * \param C Preallocated product matrix, may be NULL for automatic creation.
 * \param A Input matrix A.
 * \param B Input matrix B.
 *
 * \ingroup Multiplication
 */

mzed_t *mzed_mul_naive(mzed_t *C, const mzed_t *A, const mzed_t *B);

/**
 * \brief C such that C == AB.
 *
 * \param C Preallocated product matrix.
 * \param A Input matrix A.
 * \param B Input matrix B.
 *
 * \ingroup Multiplication
 */

mzed_t *_mzed_mul_naive(mzed_t *C, const mzed_t *A, const mzed_t *B);

/**
 * \brief C such that C == aB.
 *
 * \param C Preallocated product matrix or NULL.
 * \param a finite field element.
 * \param B Input matrix B.
 *
 * \ingroup Multiplication
 */

mzed_t *mzed_mul_scalar(mzed_t *C, const word a, const mzed_t *B);

/**
 * Check whether C, A and B match in sizes and fields for
 * multiplication
 *
 * \param C Output matrix, if NULL a new matrix is created.
 * \param A Input matrix.
 * \param B Input matrix.
 * \param clear Write zeros to C or not.
 */

mzed_t *_mzed_mul_init(mzed_t *C, const mzed_t *A, const mzed_t *B, int clear);

/**
 * \brief Fill matrix A with random elements.
 *
 * \param A Matrix
 *
 * \todo Allow the user to provide a RNG callback.
 *
 * \ingroup Assignment
 */

void mzed_randomize(mzed_t *A);

/**
 * \brief Copy matrix A to B.
 *
 * \param B May be NULL for automatic creation.
 * \param A Source matrix.
 *
 * \ingroup Assignment
 */

mzed_t *mzed_copy(mzed_t *B, const mzed_t *A);

/**
 * \brief Return diagonal matrix with value on the diagonal.
 *
 * If the matrix is not square then the largest possible square
 * submatrix is used.
 *
 * \param M Matrix
 * \param value Finite Field element
 *
 * \ingroup Assignment
 */

void mzed_set_ui(mzed_t *A, word value);


/**
 * Get the element at position (row,col) from the matrix A.
 *
 * \param A Source matrix.
 * \param row Starting row.
 * \param col Starting column.
 *
 * \ingroup Assignment
 */ 

static inline word mzed_read_elem(const mzed_t *A, const rci_t row, const rci_t col) {
  return __mzd_read_bits(A->x, row, A->w*col, A->w);
}

/**
 * At the element elem to the element at position (row,col) in the matrix A.
 *
 * \param A Target matrix.
 * \param row Starting row.
 * \param col Starting column.
 * \param elem finite field element.
 *
 * \ingroup Assignment
 */ 

static inline void mzed_add_elem(mzed_t *a, const rci_t row, const rci_t col, const word elem) {
  __mzd_xor_bits(a->x, row, a->w*col, a->w, elem);
}

/**
 * Write the element elem to the position (row,col) in the matrix A.
 *
 * \param A Target matrix.
 * \param row Starting row.
 * \param col Starting column.
 * \param elem finite field element.
 *
 * \ingroup Assignment
 */ 

static inline void mzed_write_elem(mzed_t *a, const rci_t row, const rci_t col, const word elem) {
  __mzd_clear_bits(a->x, row, a->w*col, a->w);
  __mzd_xor_bits(a->x, row, a->w*col, a->w, elem);
}

/**
 * \brief Return -1,0,1 if if A < B, A == B or A > B respectively.
 *
 * \param A Matrix.
 * \param B Matrix.
 *
 * \note This comparison is not well defined mathematically and
 * relatively arbitrary since elements of GF(2^k) don't have an
 * ordering.
 *
 * \ingroup Comparison
 */

static inline int mzed_cmp(mzed_t *A, mzed_t *B) {
  return mzd_cmp(A->x,B->x);
}


/**
 * \brief Zero test for matrix.
 *
 * \param A Input matrix.
 *
 * \ingroup Comparison
 */
static inline int mzed_is_zero(mzed_t *A) {
  return mzd_is_zero(A->x);
}

/**
 *  Compute A[ar,c] = A[ar,c] + X*B[br,c] for all c >= startcol.
 *
 * \param A Matrix.
 * \param ar Row index in A.
 * \param B Matrix.
 * \param br Row index in B.
 * \param X Lookup table for multiplication with some finite field element x.
 * \param start_col Column index.
 *
 * \ingroup RowOperations
 */

void mzed_add_multiple_of_row(mzed_t *A, rci_t ar, const mzed_t *B, rci_t br, word *X, rci_t start_col);

/**
 * \brief Recale the row r in A by X starting c.
 * 
 * \param A Matrix
 * \param r Row index.
 * \param c Column index.
 * \param X Multiplier 
 *
 * \ingroup RowOperations
 */

static inline void mzed_rescale_row(mzed_t *A, rci_t r, rci_t c, const word *X) {
  for(rci_t l=c; l<A->ncols; l++) {
    mzed_write_elem(A, r, l, X[mzed_read_elem(A, r, l)]);
  }
}

/**
 * \brief Swap the two rows rowa and rowb.
 * 
 * \param M Matrix
 * \param rowa Row index.
 * \param rowb Row index.
 *
 * \ingroup RowOperations
 *
 * \wordoffset
 */

static inline void mzed_row_swap(mzed_t *M, const rci_t rowa, const rci_t rowb) {
  mzd_row_swap(M->x, rowa, rowb);
}

/**
 * \brief copy row j from A to row i from B.
 *
 * The offsets of A and B must match and the number of columns of A
 * must be less than or equal to the number of columns of B.
 *
 * \param B Target matrix.
 * \param i Target row index.
 * \param A Source matrix.
 * \param j Source row index.
 *
 * \ingroup RowOperations
 */

static inline void mzed_copy_row(mzed_t* B, rci_t i, const mzed_t* A, rci_t j) {
  mzd_copy_row(B->x, i, A->x, j);
}

/**
 * \brief Swap the two columns cola and colb.
 * 
 * \param M Matrix.
 * \param cola Column index.
 * \param colb Column index.
 *
 * \ingroup RowOperations
 */
 
static inline void mzed_col_swap(mzed_t *M, const rci_t cola, const rci_t colb) {
  for(rci_t i=0; i<M->w; i++)
    mzd_col_swap(M->x,M->w*cola+i, M->w*colb+i);
}

/**
 * \brief Swap the two columns cola and colb but only between start_row and stop_row.
 * 
 * \param M Matrix.
 * \param cola Column index.
 * \param colb Column index.
 * \param start_row Row index.
 * \param stop_row Row index (exclusive).
 *
 * \ingroup RowOperations
 */

static inline void mzed_col_swap_in_rows(mzed_t *A, const rci_t cola, const rci_t colb, const rci_t start_row, rci_t stop_row) {
  for(int e=0; e < A->finite_field->degree; e++) {
    mzd_col_swap_in_rows(A->x, A->w*cola+e, A->w*colb+e, start_row, stop_row);
  };
}

/**
 * \brief Add the rows sourcerow and destrow and stores the total in
 * the row destrow.
 *
 * \param M Matrix
 * \param sourcerow Index of source row
 * \param destrow Index of target row
 *
 * \note this can be done much faster with mzed_combine.
 *
 * \ingroup RowOperations
 */

static inline void mzed_row_add(mzed_t *M, const rci_t sourcerow, const rci_t destrow) {
  mzd_row_add(M->x, sourcerow, destrow);
}

/**
 * \brief Return the first row with all zero entries.
 *
 * If no such row can be found returns nrows.
 *
 * \param A Matrix
 *
 * \ingroup RowOperations
 */

static inline rci_t mzed_first_zero_row(mzed_t *A) {
  return mzd_first_zero_row(A->x);
}


/**
 * \brief Clear the given row, but only begins at the column coloffset.
 *
 * \param M Matrix
 * \param row Index of row
 * \param coloffset Column offset
 *
 * \ingroup RowOperations
 */

static inline void mzed_row_clear_offset(mzed_t *M, const rci_t row, const rci_t coloffset) {
  mzd_row_clear_offset(M->x, row, coloffset*M->w);
}

/**
 * \brief Gaussian elimination.
 * 
 * Perform Gaussian elimination on the matrix A.  If full=0, then it
 * will do triangular style elimination, and if full=1, it will do
 * Gauss-Jordan style, or full elimination.
 *
 * \param A Matrix
 * \param full Gauss-Jordan style or upper unit-triangular form only.
 *
 * \ingroup Echelon
 */

rci_t mzed_echelonize_naive(mzed_t *A, int full);

/**
 * \brief Print a matrix to stdout. 
 *
 * The output will contain colons between every 4-th column.
 *
 * \param M Matrix
 *
 * \ingroup StringConversions
 */

void mzed_print(const mzed_t *M);

#endif //M4RIE_MATRIX_H
