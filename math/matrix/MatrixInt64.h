/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MATH_MATRIX_INT64_H
#define COMS_MATH_MATRIX_INT64_H

#if ARM
    #include "../../stdlib/IntrinsicsArm.h"
#else
    #include "../../stdlib/Intrinsics.h"
#endif

#endif

// Remarks: sizes for the second matrix/vector are often implied by the first parameter and the rules for matrix/vector
// multiplication.

// First element is always a matrix of int64_t
/////////////////////////////////

/////////////////////////////////
// Multiplication
/////////////////////////////////

// Array matrix multiplication
/////////////////////////////////
// mult_mat_int32(int64_t **a, size_t ai, size_t aj, int32_t **b)
// mult_mat_int64(int64_t **a, size_t ai, size_t aj, int64_t **b)
// mult_mat_float(int64_t **a, size_t ai, size_t aj, float **b)

// mult_vec_int32(int64_t **a, size_t ai, size_t aj, int32_t *b)
// mult_vec_int64(int64_t **a, size_t ai, size_t aj, int64_t *b)
// mult_vec_float(int64_t **a, size_t ai, size_t aj, float *b)

// mult_scal_int32(int64_t **a, size_t ai, size_t aj, int32_t b)
// mult_scal_int64(int64_t **a, size_t ai, size_t aj, int64_t b)
// mult_scal_float(int64_t **a, size_t ai, size_t aj, float b)

// Modifies the original matrix
// vector 2 matrix multiplication
/////////////////////////////////
// v2_mult_mat_int32(v2 *a, size_t a, int32_t **b)
// v2_mult_mat_int64(v2 *a, size_t a, int64_t **b)
// v2_mult_mat_float(v2 *a, size_t a, float **b)
// v2_mult_mat_v2(v2 *a, size_t a, v2 *b)

// v2_mult_vec_int32(v2 *a, size_t a, int32_t *b)
// v2_mult_vec_int64(v2 *a, size_t a, int64_t *b)
// v2_mult_vec_float(v2 *a, size_t a, float *b)
// v2_mult_vec_v2(v2 *a, size_t a, v2 *b)

// v2_mult_scal_int32(v2 *a, size_t a, int32_t b)
// v2_mult_scal_int64(v2 *a, size_t a, int64_t b)
// v2_mult_scal_float(v2 *a, size_t a, float b)

// vector 3 matrix multiplication
/////////////////////////////////
// v3_mult_mat_int32(v3 *a, size_t a, int32_t **b)
// v3_mult_mat_int64(v3 *a, size_t a, int64_t **b)
// v3_mult_mat_float(v3 *a, size_t a, float **b)
// v3_mult_mat_v3(v3 *a, size_t a, v3 *b)

// v3_mult_vec_int32(v3 *a, size_t a, int32_t *b)
// v3_mult_vec_int64(v3 *a, size_t a, int64_t *b)
// v3_mult_vec_float(v3 *a, size_t a, float *b)
// v3_mult_vec_v3(v3 *a, size_t a, v3 *b)

// v3_mult_scal_int32(v3 *a, size_t a, int32_t b)
// v3_mult_scal_int64(v3 *a, size_t a, int64_t b)
// v3_mult_scal_float(v3 *a, size_t a, float b)

// vector 4 matrix multiplication
/////////////////////////////////
// v4_mult_mat_int32(v4 *a, size_t a, int32_t **b)
// v4_mult_mat_int64(v4 *a, size_t a, int64_t **b)
// v4_mult_mat_float(v4 *a, size_t a, float **b)
// v4_mult_mat_v4(v4 *a, size_t a, v4 *b)

// v4_mult_vec_int32(v4 *a, size_t a, int32_t *b)
// v4_mult_vec_int64(v4 *a, size_t a, int64_t *b)
// v4_mult_vec_float(v4 *a, size_t a, float *b)
// v4_mult_vec_v4(v4 *a, size_t a, v4 *b)

// v4_mult_scal_int32(v4 *a, size_t a, int32_t b)
// v4_mult_scal_int64(v4 *a, size_t a, int64_t b)
// v4_mult_scal_float(v4 *a, size_t a, float b)

/////////////////////////////////
// Addition
/////////////////////////////////

// Array matrix addition
/////////////////////////////////
// add_mat_int32(int64_t **a, size_t ai, size_t aj, int32_t **b)
// add_mat_int64(int64_t **a, size_t ai, size_t aj, int64_t **b)
// add_mat_float(int64_t **a, size_t ai, size_t aj, float **b)

// add_vec_int32(int64_t **a, size_t ai, size_t aj, int32_t *b)
// add_vec_int64(int64_t **a, size_t ai, size_t aj, int64_t *b)
// add_vec_float(int64_t **a, size_t ai, size_t aj, float *b)

// add_scal_int32(int64_t **a, size_t ai, size_t aj, int32_t b)
// add_scal_int64(int64_t **a, size_t ai, size_t aj, int64_t b)
// add_scal_float(int64_t **a, size_t ai, size_t aj, float b)

// vector 2 matrix addition
/////////////////////////////////
// v2_add_mat_int32(v2 *a, size_t a, int32_t **b)
// v2_add_mat_int64(v2 *a, size_t a, int64_t **b)
// v2_add_mat_float(v2 *a, size_t a, float **b)
// v2_add_mat_v2(v2 *a, size_t a, v2 *b)

// v2_add_vec_int32(v2 *a, size_t a, int32_t *b)
// v2_add_vec_int64(v2 *a, size_t a, int64_t *b)
// v2_add_vec_float(v2 *a, size_t a, float *b)
// v2_add_vec_v2(v2 *a, size_t a, v2 *b)

// v2_add_scal_int32(v2 *a, size_t a, int32_t b)
// v2_add_scal_int64(v2 *a, size_t a, int64_t b)
// v2_add_scal_float(v2 *a, size_t a, float b)

// vector 3 matrix addition
/////////////////////////////////
// v3_add_mat_int32(v3 *a, size_t a, int32_t **b)
// v3_add_mat_int64(v3 *a, size_t a, int64_t **b)
// v3_add_mat_float(v3 *a, size_t a, float **b)
// v3_add_mat_v3(v3 *a, size_t a, v3 *b)

// v3_add_vec_int32(v3 *a, size_t a, int32_t *b)
// v3_add_vec_int64(v3 *a, size_t a, int64_t *b)
// v3_add_vec_float(v3 *a, size_t a, float *b)
// v3_add_vec_v3(v3 *a, size_t a, v3 *b)

// v3_add_scal_int32(v3 *a, size_t a, int32_t b)
// v3_add_scal_int64(v3 *a, size_t a, int64_t b)
// v3_add_scal_float(v3 *a, size_t a, float b)

// vector 4 matrix addition
/////////////////////////////////
// v4_add_mat_int32(v4 *a, size_t a, int32_t **b)
// v4_add_mat_int64(v4 *a, size_t a, int64_t **b)
// v4_add_mat_float(v4 *a, size_t a, float **b)
// v4_add_mat_v4(v4 *a, size_t a, v4 *b)

// v4_add_vec_int32(v4 *a, size_t a, int32_t *b)
// v4_add_vec_int64(v4 *a, size_t a, int64_t *b)
// v4_add_vec_float(v4 *a, size_t a, float *b)
// v4_add_vec_v4(v4 *a, size_t a, v4 *b)

// v4_add_scal_int32(v4 *a, size_t a, int32_t b)
// v4_add_scal_int64(v4 *a, size_t a, int64_t b)
// v4_add_scal_float(v4 *a, size_t a, float b)

/////////////////////////////////
// Subtraction
/////////////////////////////////

// Array matrix subtraction
/////////////////////////////////
// sub_mat_int32(int64_t **a, size_t ai, size_t aj, int32_t **b)
// sub_mat_int64(int64_t **a, size_t ai, size_t aj, int64_t **b)
// sub_mat_float(int64_t **a, size_t ai, size_t aj, float **b)

// sub_vec_int32(int64_t **a, size_t ai, size_t aj, int32_t *b)
// sub_vec_int64(int64_t **a, size_t ai, size_t aj, int64_t *b)
// sub_vec_float(int64_t **a, size_t ai, size_t aj, float *b)

// sub_scal_int32(int64_t **a, size_t ai, size_t aj, int32_t b)
// sub_scal_int64(int64_t **a, size_t ai, size_t aj, int64_t b)
// sub_scal_float(int64_t **a, size_t ai, size_t aj, float b)

// vector 2 matrix subtraction
/////////////////////////////////
// v2_sub_mat_int32(v2 *a, size_t a, int32_t **b)
// v2_sub_mat_int64(v2 *a, size_t a, int64_t **b)
// v2_sub_mat_float(v2 *a, size_t a, float **b)
// v2_sub_mat_v2(v2 *a, size_t a, v2 *b)

// v2_sub_vec_int32(v2 *a, size_t a, int32_t *b)
// v2_sub_vec_int64(v2 *a, size_t a, int64_t *b)
// v2_sub_vec_float(v2 *a, size_t a, float *b)
// v2_sub_vec_v2(v2 *a, size_t a, v2 *b)

// v2_sub_scal_int32(v2 *a, size_t a, int32_t b)
// v2_sub_scal_int64(v2 *a, size_t a, int64_t b)
// v2_sub_scal_float(v2 *a, size_t a, float b)

// vector 3 matrix subtraction
/////////////////////////////////
// v3_sub_mat_int32(v3 *a, size_t a, int32_t **b)
// v3_sub_mat_int64(v3 *a, size_t a, int64_t **b)
// v3_sub_mat_float(v3 *a, size_t a, float **b)
// v3_sub_mat_v3(v3 *a, size_t a, v3 *b)

// v3_sub_vec_int32(v3 *a, size_t a, int32_t *b)
// v3_sub_vec_int64(v3 *a, size_t a, int64_t *b)
// v3_sub_vec_float(v3 *a, size_t a, float *b)
// v3_sub_vec_v3(v3 *a, size_t a, v3 *b)

// v3_sub_scal_int32(v3 *a, size_t a, int32_t b)
// v3_sub_scal_int64(v3 *a, size_t a, int64_t b)
// v3_sub_scal_float(v3 *a, size_t a, float b)

// vector 4 matrix subtraction
/////////////////////////////////
// v4_sub_mat_int32(v4 *a, size_t a, int32_t **b)
// v4_sub_mat_int64(v4 *a, size_t a, int64_t **b)
// v4_sub_mat_float(v4 *a, size_t a, float **b)
// v4_sub_mat_v4(v4 *a, size_t a, v4 *b)

// v4_sub_vec_int32(v4 *a, size_t a, int32_t *b)
// v4_sub_vec_int64(v4 *a, size_t a, int64_t *b)
// v4_sub_vec_float(v4 *a, size_t a, float *b)
// v4_sub_vec_v4(v4 *a, size_t a, v4 *b)

// v4_sub_scal_int32(v4 *a, size_t a, int32_t b)
// v4_sub_scal_int64(v4 *a, size_t a, int64_t b)
// v4_sub_scal_float(v4 *a, size_t a, float b)
