// Implementation for 4x4 transformation class
// contains a 4x4 transformation matrix and its inverse
//
// static build functions:
//	 create(mat, inv): build from a matrix and its inverse
//	 identity(): both matrix and inverse are 4x4 identity
//	 xrotate(angle): matrix & inverse to rotate angle radians around x axis
//	 yrotate(angle): matrix & inverse to rotate angle radians around y axis
//	 zrotate(angle): matrix & inverse to rotate angle radians around z axis
//	 translate(Vec<3> t): matrix & inverse to translate by vector t
//	 perspective(fov, aspect, near, far): 4x4 perspective matrix & inverse
// use these as e.g. use as Xform::identity()
//
// Linear algebra functions (m1, m2 = matrices)
//	 m1*m2: Performs matrix multiply on matrix and inverse (w/ reversed order)

#include "Xform.hpp"
#include "Mat.inl"
#include <math.h>

//////////////////////////////////////////////////////////////////////
// 4x4 transformation matrix/inverse pairs

// construct pair from matrix and inverse
Xform Xform::create(const Mat<4> &mat, const Mat<4> &inv) {
	Xform mp = {mat, inv};
	return mp;
}

// matrix/matrix multiply
Xform operator*(const Xform &m1, const Xform &m2) {
	return Xform::create(m1.matrix * m2.matrix, m2.inverse * m1.inverse);
}

// identity
Xform Xform::identity() {
	return create(Mat4(Vec4(1.f, 0.f, 0.f, 0.f),
					   Vec4(0.f, 1.f, 0.f, 0.f),
					   Vec4(0.f, 0.f, 1.f, 0.f),
					   Vec4(0.f, 0.f, 0.f, 1.f)),
				  Mat4(Vec4(1.f, 0.f, 0.f, 0.f),
					   Vec4(0.f, 1.f, 0.f, 0.f),
					   Vec4(0.f, 0.f, 1.f, 0.f),
					   Vec4(0.f, 0.f, 0.f, 1.f)));
}

// build x, y, or z axis rotation for angle (in radians)
Xform Xform::xrotate(float angle) {
	float c = cosf(angle), s = sinf(angle);
	return create(Mat4(Vec4(1.f, 0.f, 0.f, 0.f),
					   Vec4(0.f,  c,   s,  0.f),
					   Vec4(0.f, -s,   c,  0.f),
					   Vec4(0.f, 0.f, 0.f, 1.f)),
				  Mat4(Vec4(1.f, 0.f, 0.f, 0.f),
					   Vec4(0.f,  c,  -s,  0.f),
					   Vec4(0.f,  s,   c,  0.f),
					   Vec4(0.f, 0.f, 0.f, 1.f)));
}
Xform Xform::yrotate(float angle) {
	float c = cosf(angle), s = sinf(angle);
	return create(Mat4(Vec4( c,	 0.f, -s,  0.f),
					   Vec4(0.f, 1.f, 0.f, 0.f),
					   Vec4( s,	 0.f,  c,  0.f),
					   Vec4(0.f, 0.f, 0.f, 1.f)),
				  Mat4(Vec4( c,	 0.f,  s,  0.f),
					   Vec4(0.f, 1.f, 0.f, 0.f),
					   Vec4(-s,	 0.f,  c,  0.f),
					   Vec4(0.f, 0.f, 0.f, 1.f)));
}
Xform Xform::zrotate(float angle) {
	float c = cosf(angle), s = sinf(angle);
	return create(Mat4(Vec4( c,	  s,  0.f, 0.f),
					   Vec4(-s,	  c,  0.f, 0.f),
					   Vec4(0.f, 0.f, 1.f, 0.f),
					   Vec4(0.f, 0.f, 0.f, 1.f)),
				  Mat4(Vec4( c,	 -s,  0.f, 0.f),
					   Vec4( s,	  c,  0.f, 0.f),
					   Vec4(0.f, 0.f, 1.f, 0.f),
					   Vec4(0.f, 0.f, 0.f, 1.f)));
}

// build 4x4 translation matrix
Xform Xform::translate(Vec<3> t) {
	return create(Mat4(Vec4(1.f,  0.f,	0.f,  0.f),
					   Vec4(0.f,  1.f,	0.f,  0.f),
					   Vec4(0.f,  0.f,	1.f,  0.f),
					   Vec4(t.x,  t.y,	t.z,  1.f)),
				  Mat4(Vec4(1.f,  0.f,	0.f,  0.f),
					   Vec4(0.f,  1.f,	0.f,  0.f),
					   Vec4(0.f,  0.f,	1.f,  0.f),
					   Vec4(-t.x, -t.y, -t.z, 1.f)));
}

// build 4x4 scale matrix
Xform Xform::scale(Vec<3> s) {
	return create(Mat4(Vec4(s.x, 0.f, 0.f, 0.f),
					   Vec4(0.f, s.y, 0.f, 0.f),
					   Vec4(0.f, 0.f, s.z, 0.f),
					   Vec4(0.f, 0.f, 0.f, 1.f)),
				  Mat4(Vec4(1.f/s.x,   0.f,		0.f,   0.f),
					   Vec4(  0.f,	 1.f/s.y,	0.f,   0.f),
					   Vec4(  0.f,	   0.f,	  1.f/s.z, 0.f),
					   Vec4(  0.f,	   0.f,		0.f,   1.f)));
}

// build 4x4 perspective from field of view (in radians),
// x/y aspect ratio, and near/far clipping planes
Xform Xform::perspective(float fov, float aspect, float near, float far) {
	float t = tanf(0.5f * fov), ta = t*aspect;
	float npf = near+far, nmf = near-far, nf2 = near*far*2.f;
	float nmfi = 1.f/nmf, nf2i = 1.f/nf2;
	return create(Mat4(Vec4(1.f/ta,	 0.f,	 0.f,	  0.f),
					   Vec4(  0.f, 1.f/t,	 0.f,	  0.f),
					   Vec4(  0.f,	 0.f,  npf*nmfi, -1.f),
					   Vec4(  0.f,	 0.f,  nf2*nmfi,  0.f)),
				  Mat4(Vec4(ta,	 0.f,  0.f,	  0.f	),
					   Vec4(0.f,  t,   0.f,	  0.f	),
					   Vec4(0.f, 0.f,  0.f, nmf*nf2i),
					   Vec4(0.f, 0.f, -1.f, npf*nf2i)));
}
