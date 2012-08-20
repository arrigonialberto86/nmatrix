/////////////////////////////////////////////////////////////////////
// = NMatrix
//
// A linear algebra library for scientific computation in Ruby.
// NMatrix is part of SciRuby.
//
// NMatrix was originally inspired by and derived from NArray, by
// Masahiro Tanaka: http://narray.rubyforge.org
//
// == Copyright Information
//
// SciRuby is Copyright (c) 2010 - 2012, Ruby Science Foundation
// NMatrix is Copyright (c) 2012, Ruby Science Foundation
//
// Please see LICENSE.txt for additional copyright notices.
//
// == Contributing
//
// By contributing source code to SciRuby, you agree to be bound by
// our Contributor Agreement:
//
// * https://github.com/SciRuby/sciruby/wiki/Contributor-Agreement
//
// == yale.h
//
// "new yale" storage format for 2D matrices (like yale, but with
// the diagonal pulled out for O(1) access).
//
// Specifications:
// * dtype and index dtype must necessarily differ
//      * index dtype is defined by whatever unsigned type can store
//        max(rows,cols)
//      * that means vector ija stores only index dtype, but a stores
//        dtype
// * vectors must be able to grow as necessary
//      * maximum size is rows*cols+1

#ifndef YALE_H
#define YALE_H

/*
 * Standard Includes
 */

#include <limits> // for std::numeric_limits<T>::max()

/*
 * Project Includes
 */

#include "types.h"

#include "data/data.h"

#include "common.h"

#include "nmatrix.hpp"

/*
 * Macros
 */

#define YALE_GROWTH_CONSTANT 1.5

//#define YALE_JA_START(sptr)             (((YALE_STORAGE*)(sptr))->shape[0]+1)
#define YALE_IJA(sptr,elem_size,i)          (void*)( (char*)(((YALE_STORAGE*)(sptr))->ija) + i * elem_size )
//#define YALE_JA(sptr,dtype,j)           ((((dtype)*)((YALE_STORAGE*)(sptr))->ija)[(YALE_JA_START(sptr))+j])
#define YALE_ROW_LENGTH(sptr,elem_size,i)   (*(size_t*)YALE_IA((sptr),(elem_size),(i)+1) - *(size_t*)YALE_IJA((sptr),(elem_size),(i)))
#define YALE_A(sptr,elem_size,i)            (void*)((char*)(((YALE_STORAGE*)(sptr))->a) + elem_size * i)
#define YALE_DIAG(sptr, elem_size, i)       ( YALE_A((sptr),(elem_size),(i)) )
//#define YALE_LU(sptr,dtype,i,j)             (((dtype)*)(((YALE_STORAGE*)(sptr))->a)[ YALE_JA_START(sptr) +  ])
#define YALE_MINIMUM(sptr)                  (((YALE_STORAGE*)(sptr))->shape[0]*2 + 1) // arbitrarily defined
#define YALE_SIZE_PTR(sptr,elem_size)       (void*)((char*)((YALE_STORAGE*)(sptr))->ija + ((YALE_STORAGE*)(sptr))->shape[0]*elem_size )
#define YALE_MAX_SIZE(sptr)                 (((YALE_STORAGE*)(sptr))->shape[0] * ((YALE_STORAGE*)(sptr))->shape[1] + 1)
#define YALE_IA_SIZE(sptr)                  ((YALE_STORAGE*)(sptr))->shape[0]

// None of these next three return anything. They set a reference directly.
#define YaleGetIJA(victim,s,i)              //(SetFuncs[Y_SIZE_T][(s)->itype](1, &(victim), 0, YALE_IJA((s), DTYPE_SIZES[s->itype], (i)), 0))
#define YaleSetIJA(i,s,from)                //(SetFuncs[s->itype][Y_SIZE_T](1, YALE_IJA((s), DTYPE_SIZES[s->itype], (i)), 0, &(from), 0))
#define YaleGetSize(sz,s)                   //(SetFuncs[Y_SIZE_T][((YALE_STORAGE*)s)->itype](1, &sz, 0, (YALE_SIZE_PTR(((YALE_STORAGE*)s), DTYPE_SIZES[((YALE_STORAGE*)s)->itype])), 0))
//#define YALE_FIRST_NZ_ROW_ENTRY(sptr,elem_size,i)

#ifndef NM_CHECK_ALLOC
 #define NM_CHECK_ALLOC(x) if (!x) rb_raise(rb_eNoMemError, "insufficient memory");
#endif
/*
 * Types
 */


/*
 * Data
 */
 

/*
 * Functions
 */

///////////////
// Lifecycle //
///////////////

YALE_STORAGE* yale_storage_create(dtype_t dtype, size_t* shape, size_t rank, size_t init_capacity);
YALE_STORAGE* yale_storage_create_from_old_yale(dtype_t dtype, size_t* shape, void* ia, void* ja, void* a, dtype_t from_dtype);
YALE_STORAGE*	yale_storage_create_merged(const YALE_STORAGE* merge_template, const YALE_STORAGE* other);
void          yale_storage_delete(STORAGE* s);
void					yale_storage_init(YALE_STORAGE* s);
void					yale_storage_mark(void*);

///////////////
// Accessors //
///////////////

void* yale_storage_get(STORAGE* s, SLICE* slice);
void*	yale_storage_ref(STORAGE* s, SLICE* slice);
char  yale_storage_set(STORAGE* storage, SLICE* slice, void* v);

inline size_t  yale_storage_get_size(const YALE_STORAGE* storage);

///////////
// Tests //
///////////

bool yale_storage_eqeq(const STORAGE* left, const STORAGE* right);

//////////
// Math //
//////////

STORAGE* yale_storage_ew_multiply(const STORAGE* left, const STORAGE* right);
STORAGE* yale_storage_matrix_multiply(const STORAGE_PAIR& casted_storage, size_t* resulting_shape, bool vector);

/////////////
// Utility //
/////////////

/*
 * Documentation goes here.
 */
// (((YALE_STORAGE*)(sptr))->shape[0] * ((YALE_STORAGE*)(sptr))->shape[1] + 1)
inline itype_t yale_storage_itype_by_shape(const size_t* shape) {
  uint64_t yale_max_size = shape[0] * (shape[1]+1);

  if (yale_max_size < static_cast<uint64_t>(std::numeric_limits<uint8_t>::max()) - 2) {
    return UINT8;

  } else if (yale_max_size < static_cast<uint64_t>(std::numeric_limits<uint16_t>::max()) - 2) {
    return UINT16;

  } else if (yale_max_size < std::numeric_limits<uint32_t>::max() - 2) {
    return UINT32;

  } else {
    return UINT64;
  }
}

/*
 * Determine the index dtype (which will be used for the ija vector). This is
 * determined by matrix shape, not IJA/A vector capacity. Note that it's MAX-2
 * because UINTX_MAX and UINTX_MAX-1 are both reserved for sparse matrix
 * multiplication.
 */
inline itype_t yale_storage_itype(const YALE_STORAGE* s) {
  return yale_storage_itype_by_shape(s->shape);
}

void		yale_storage_print_vectors(YALE_STORAGE* s);

template <typename IType>
int yale_storage_binary_search_template(YALE_STORAGE* s, IType left, IType right, IType key);

template <typename DType, typename IType>
char yale_storage_vector_insert_resize_template(YALE_STORAGE* s, size_t current_size, size_t pos, size_t* j, size_t n, bool struct_only);

template <typename DType, typename IType>
char yale_storage_vector_insert_template(YALE_STORAGE* s, size_t pos, size_t* j, DType* val, size_t n, bool struct_only);

/*
 * Clear out the D portion of the A vector (clearing the diagonal and setting
 * the zero value).
 *
 * Note: This sets a literal 0 value. If your dtype is RUBYOBJ (a Ruby object),
 * it'll actually be INT2FIX(0) instead of a string of NULLs.
 */
template <typename DType>
inline void yale_storage_clear_diagonal_and_zero_template(YALE_STORAGE* s) {
  DType* a = reinterpret_cast<DType*>(s->a);

  // Clear out the diagonal + one extra entry
  for (size_t i = 0; i < s->shape[0]+1; ++i) // insert Ruby zeros
    a[i] = 0;
}

/////////////////////////
// Copying and Casting //
/////////////////////////

STORAGE*      yale_storage_cast_copy(const STORAGE* rhs, dtype_t new_dtype);
YALE_STORAGE* yale_storage_copy(YALE_STORAGE* rhs);
STORAGE*      yale_storage_copy_transposed(const STORAGE* rhs_base);

template <typename IType>
void						yale_storage_clear_diagonal_and_zero_template(YALE_STORAGE* s);

/*template <typename IType>
void yale_storage_clear_diagonal_and_zero_template(YALE_STORAGE* s);*/


void Init_yale_functions(void);

#endif // YALE_H
