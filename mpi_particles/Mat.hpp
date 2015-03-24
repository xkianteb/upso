// NxN matrix class
//
// Classes:
//	 Mat<N>: NxN-dimensional float matrix
//	 MatPair<N>: NxN matrix and inverse
//
// This file contains only minimal class definitions for reduced build
// dependency and faster build times when included in other headers.
// All operators are defined as inline functions in Mat.inl. Include
// Mat.hpp in headers with matrix members, and Mat.inl in code files
// that need to use or manipulate the matrices.
//
// These classes are initialized using build functions rather than with
// a constructor so they will be considered a POD (Plain Old Data) type
// Among other things, this guarantees there won't be any extra vtable
// or other added data changing the memory layout, and that you can
// create an uninitialized array without the compiler running the
// default constructor on all elements.

#ifndef Mat_hpp
#define Mat_hpp

#include "Vec.hpp"

template <int N>
struct Mat {
	float data[N][N];

	// array-like access must be public member functions
	// mat[i] returns Vec<N> for column i, mat[i][j] will be individual element
	// all other operations as template functions in Mat.inl
	Vec<N> operator[](int i) const { return *(Vec<N>*)data[i]; }
	Vec<N> &operator[](int i) { return *(Vec<N>*)data[i]; }
};

#endif
