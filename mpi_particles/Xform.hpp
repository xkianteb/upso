// Transformation class
// contains both a 4x4 matrix and its inverse

#ifndef Xform_hpp
#define Xform_hpp

#include "Mat.hpp"
#include "Vec.hpp"

// transformation as a pair of 4x4 matrices (matrix and inverse)
struct Xform {
	Mat<4> matrix, inverse;

	// declared static, so can call as Xform::create(...)
	static Xform create(const Mat<4> &xform, const Mat<4> &invxform);
	
	// Xform::identity() to make an identity matrix
	static Xform identity();

	// rotations around x, y or z axis
	static Xform xrotate(float angle);
	static Xform yrotate(float angle);
	static Xform zrotate(float angle);

	// new translation matrix
	static Xform translate(Vec<3> t);

	// new scale matrix
	static Xform scale(Vec<3> s);

	// new perspective matrix from field of view (in radians),
	// x/y aspect ratio, and near/far clipping planes
	static Xform perspective(float fov, float aspect, float near, float far);
};

// multipliy matrices, multiply inverses in reverse order
Xform operator*(const Xform &m1, const Xform &m2);

#endif
