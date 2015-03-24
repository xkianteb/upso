// N-dimensional vector class template inline functions
// based on ideas from Nathan Reed
//	  http://www.reedbeta.com/blog/2013/12/28/on-vector-math-libraries/
//
// This file contains function declarations and inline function definitions.
// The code is mostly boring and repetitive. If you need a function and it
// isn't here, go ahead and add it!
//
// Note that since the size is a template parameter, it's a compile-time
// constant. Loops with N iterations should be unrolled by the compiler
//
// Build functions
//	 Vec<2> Vec2(x,y): construct Vec<2>
//	 Vec<3> Vec3(x,y,z): construct a Vec<3>
//	 Vec<4> Vec4(x,y,z,w): construct a Vec<4>
//
// Math operations that apply to each element (s = scalar; v, v1, v2 = vectors)
//	 v1+v2, v1-v2, v1*v2, v1/v2
//	 -v, s+v, v+s, s-v, v-s, s*v, v*s, s/v, v/s
//
// Linear algebra and other handy functions
//	 float dot(v1,v2): N-dimensional vector dot product
//	 float length(v): Euclidean vector length
//	 Vec<N> normalize(v): unit-length vector in the direction of v
//	 v1^v2: vector cross product -- v1 and v2 must both be Vec<3>
//	 dimensions(v): size of N (e.g. 2 for Vec<2>, 3 for Vec<3>, etc.)

#ifndef Vec_inl
#define Vec_inl

#include "Vec.hpp"
#include <math.h>

//////////////////////////////////////////////////////////////////////
// Build functions. Not done as constructors, so the vector will be a
// POD (plain old data) type for pre-C++11. Among other things,
// guarantees the type will be exactly equivalent to an array in
// memory.

// build 2D, 3D or 4D from a list of components
inline Vec<2> Vec2(float x, float y) {
	Vec<2> v;
	v[0] = x; v[1] = y;
	return v;
}

inline Vec<3> Vec3(float x, float y, float z) {
	Vec<3> v;
	v[0] = x; v[1] = y; v[2] = z;
	return v;
}

inline Vec<4> Vec4(float x, float y, float z, float w) {
	Vec<4> v;
	v[0] = x; v[1] = y; v[2] = z; v[3] = w;
	return v;
}

//////////////////////////////////////////////////////////////////////
// component-wise operations: +, -, *, /, %
template <int N>
inline Vec<N> operator+(const Vec<N> &v1, const Vec<N> &v2) {
	Vec<N> v;
	for(int i=0; i<N; ++i)
		v[i] = v1[i] + v2[i];
	return v;
}

template <int N>
inline Vec<N> operator-(const Vec<N> &v1, const Vec<N> &v2) {
	Vec<N> v;
	for(int i=0; i<N; ++i)
		v[i] = v1[i] - v2[i];
	return v;
}

template <int N>
inline Vec<N> operator*(const Vec<N> &v1, const Vec<N> &v2) {
	Vec<N> v;
	for(int i=0; i<N; ++i)
		v[i] = v1[i] * v2[i];
	return v;
}

template <int N>
inline Vec<N> operator/(const Vec<N> &v1, const Vec<N> &v2) {
	Vec<N> v;
	for(int i=0; i<N; ++i)
		v[i] = v1[i] / v2[i];
	return v;
}

//////////////////////////////////////////////////
// operations between a scalar and vector: +, -, *, /
template <int N>
inline Vec<N> operator-(const Vec<N> &v1) {
	Vec<N> v;
	for(int i=0; i<N; ++i)
		v[i] = -v1[i];
	return v;
}

template <int N>
inline Vec<N> operator+(float s, const Vec<N> &v1) {
	Vec<N> v;
	for(int i=0; i<N; ++i)
		v[i] = s + v1[i];
	return v;
}

template <int N>
inline Vec<N> operator-(float s, const Vec<N> &v1) {
	Vec<N> v;
	for(int i=0; i<N; ++i)
		v[i] = s - v1[i];
	return v;
}

template <int N>
inline Vec<N> operator*(float s, const Vec<N> &v1) {
	Vec<N> v;
	for(int i=0; i<N; ++i)
		v[i] = s * v1[i];
	return v;
}

template <int N>
inline Vec<N> operator/(float s, const Vec<N> &v1) {
	Vec<N> v;
	for(int i=0; i<N; ++i)
		v[i] = s / v1[i];
	return v;
}

//////////////////////////////////////////////////
// operations between a vector and a scalar: +, -, *, /
template <int N>
inline Vec<N> operator+(const Vec<N> &v1, float s) {
	Vec<N> v;
	for(int i=0; i<N; ++i)
		v[i] = v1[i] + s;
	return v;
}

template <int N>
inline Vec<N> operator-(const Vec<N> &v1, float s) {
	Vec<N> v;
	for(int i=0; i<N; ++i)
		v[i] = v1[i] - s;
	return v;
}

template <int N>
inline Vec<N> operator*(const Vec<N> &v1, float s) {
	Vec<N> v;
	for(int i=0; i<N; ++i)
		v[i] = v1[i] * s;
	return v;
}

template <int N>
inline Vec<N> operator/(const Vec<N> &v1, float s) {
	Vec<N> v;
	for(int i=0; i<N; ++i)
		v[i] = v1[i] / s;
	return v;
}

//////////////////////////////////////////////////////////////////////
// other linear algebra operations

// vector dot product, dot(v1, v2)
template <int N>
inline float dot(const Vec<N> &v1, const Vec<N> &v2) {
	float v = 0;
	for(int i=0; i<N; ++i)
		v += v1[i] * v2[i];
	return v;
}

// vector length, length(v1)
template <int N>
inline float length(const Vec< N> &v1) {
	return sqrtf(dot(v1, v1));
}

// normalized vector, normalize(v1)
template <int N>
inline Vec<N> normalize(const Vec<N> &v1) {
	return v1 / length(v1);
}

// cross product, v1^v2 for Vec<3> only
inline Vec<3> operator^(const Vec<3> &v1, const Vec<3> &v2) {
	Vec<3> v;
	v[0] = v1[1]*v2[2] - v1[2]*v2[1];
	v[1] = v1[2]*v2[0] - v1[0]*v2[2];
	v[2] = v1[0]*v2[1] - v1[1]*v2[0];
	return v;
}

template <int N>
inline unsigned int dimensions(const Vec<N> &v) {
	return N;
}

#endif
