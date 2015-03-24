// N-dimensional template vector class
// based on ideas from Nathan Reed
//	  http://www.reedbeta.com/blog/2013/12/28/on-vector-math-libraries/
//
// Classes:
//	 Vec<N>: N-dimensional float vector
//
// This file contains only minimal class definitions for reduced build
// dependency and faster build times when included in other headers.
// All operators are defined as inline functions in Vec.inl. Include
// Vec.hpp in headers with vector members, and Vec.inl in code files
// that need to use or manipulate the vectors.
//
// These classes are initialized using build functions rather than with
// a constructor so they will be considered a POD (Plain Old Data) type
// Among other things, this guarantees there won't be any extra vtable
// or other added data changing the memory layout, and that you can
// create an uninitialized array without the compiler running the
// default constructor on all elements.

#ifndef Vec_hpp
#define Vec_hpp

//////////////////////////////////////////////////////////////////////
// N-dimensional vector of any component type
template <int N>
struct Vec {
	float data[N];

	// array-like access must be public member functions
	// all other operations as template functions in Vec.inl
	float operator[](int i) const { return data[i]; }
	float &operator[](int i) { return data[i]; }
};

//////////////////////////////////////////////////////////////////////
// 2D, 3D and 4D specialization with component access
template <>
struct Vec<2> {
	union {
		float data[2];
		struct { float x, y; };
	};
	float operator[](int i) const { return data[i]; }
	float &operator[](int i) { return data[i]; }
};

template <>
struct Vec<3> {
	union {
		float data[3];
		struct { float x, y, z; };
		Vec<2> xy;
	};
	float operator[](int i) const { return data[i]; }
	float &operator[](int i) { return data[i]; }
};

template <>
struct Vec<4> {
	union {
		float data[4];
		struct { float x, y, z, w; };
		Vec<2> xy;
		Vec<3> xyz;
	};
	float operator[](int i) const { return data[i]; }
	float &operator[](int i) { return data[i]; }
};

#endif
