// N x N template matrix class inline implementation
//
// This file contains inline function definitions. The code is mostly
// boring and repetitive. If you need a function and it isn't here, go
// ahead and add it!
//
// Build functions
//	 Mat2(c0, c1): build Mat<2> from Vec<2> columns
//	 Mat3(c0, c1, c2): build Mat<3> from Vec<3> columns
//	 Mat4(c0, c1, c2, c3): build Mat<4> from Vec<4> columns
//
// Linear algebra functions (s = scalar; v = vector, m, m1, m2 = matrices)
//	 -m, s*m, m*s, m/s, m1+m2, m1-m2, m1*m2, v*m, m*v
//		: usual linear algebra meanings (e.g. m1*m2 is a matrix multiply)
//	 transpose(m): matrix transpose

#ifndef Mat_inl
#define Mat_inl

#include "Mat.hpp"
#include "Vec.inl"

//////////////////////////////////////////////////////////////////////
// Build matrices

// build by column
inline Mat<2> Mat2(Vec<2> c0, Vec<2> c1) {
	Mat<2> m;
	m[0] = c0; m[1] = c1;
	return m;
}
inline Mat<3> Mat3(Vec<3> c0, Vec<3> c1, Vec<3> c2) {
	Mat<3> m;
	m[0] = c0; m[1] = c1; m[2] = c2;
	return m;
}
inline Mat<4> Mat4(Vec<4> c0, Vec<4> c1, Vec<4> c2, Vec<4> c3) {
	Mat<4> m;
	m[0] = c0; m[1] = c1; m[2] = c2; m[3] = c3;
	return m;
}

//////////////////////////////////////////////////////////////////////
// matrix addition and subtraction
template <int N>
inline Mat<N> operator+(const Mat<N> &m1, const Mat<N> &m2) {
	Mat<N> m;
	for(int i=0; i!=N; ++i)
		for(int j=0; j!=N; ++j)
			m[i][j] = m1[i][j] + m2[i][j];
	return m;
}

template <int N>
inline Mat<N> operator-(const Mat<N> &m1, const Mat<N> &m2) {
	Mat<N> m;
	for(int i=0; i!=N; ++i)
		for(int j=0; j!=N; ++j)
			m[i][j] = m1[i][j] - m2[i][j];
	return m;
}

//////////////////////////////////////////////////////////////////////
// scalar multiplication and division
template <int N>
inline Mat<N> operator-(const Mat<N> &m1) {
	Mat<N> m;
	for(int i=0; i!=N; ++i)
		for(int j=0; j!=N; ++j)
			m[i][j] = -m1[i][j];
	return m;
}

template <int N>
inline Mat<N> operator*(float s, const Mat<N> &m1) {
	Mat<N> m;
	for(int i=0; i!=N; ++i)
		for(int j=0; j!=N; ++j)
			m[i][j] = s * m1[i][j];
	return m;
}

template <int N>
inline Mat<N> operator*(const Mat<N> &m1, float s) {
	Mat<N> m;
	for(int i=0; i!=N; ++i)
		for(int j=0; j!=N; ++j)
			m[i][j] = m1[i][j] * s;
	return m;
}

template <int N>
inline Mat<N> operator/(const Mat<N> &m1, float s) {
	Mat<N> m;
	for(int i=0; i!=N; ++i)
		for(int j=0; j!=N; ++j)
			m[i][j] = m1[i][j] / s;
	return m;
}

//////////////////////////////////////////////////////////////////////
// matrix*matrix, matrix*vector, and vector*matrix
template <int N>
inline Mat<N> operator*(const Mat<N> &m1, const Mat<N> &m2) {
	Mat<N> m;
	for(int i=0; i!=N; ++i) {
		for(int j=0; j!=N; ++j) {
			m[j][i] = m2[j][0] * m1[0][i];

			for(int k=1; k!=N; ++k)
				m[j][i] += m2[j][k] * m1[k][i];
		}
	}
	return m;
}

template <int N>
inline Vec<N> operator*(const Vec<N> &v1, const Mat<N> &m1) {
	Vec<N> v;
	for(int i=0; i!=N; ++i) {
		v[i] = m1[i][0] * v1[0];

		for(int j=1; j!=N; ++j)
			v[i] += m1[i][j] * v1[j];
	}
	return v;
}

template <int N>
inline Vec<N> operator*(const Mat<N> &m1, const Vec<N> &v1) {
	Vec<N> v;
	for(int j=0; j!=N; ++j) {
		v[j] = v1[0] * m1[0][j];

		for(int i=1; i!=N; ++i)
			v[j] += v1[i] * m1[i][j];
	}
	return v;
}

//////////////////////////////////////////////////////////////////////
// transpose
template <int N>
inline Mat<N> transpose(const Mat<N> &m1) {
	Mat<N> m;
	for(int i=0; i != N; ++i)
		for(int j=0; j != N; ++j)
			m[i][j] = m1[j][i];
	return m;
}

#endif
