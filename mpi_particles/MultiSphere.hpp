// add sphere object to draw list
#ifndef MultiSphere_hpp
#define MultiSphere_hpp

#include "Vec.hpp"

struct MultiSphere {
	unsigned int drawID;
	unsigned int num_verts_per_sphere;
	unsigned int num_spheres;

	// create spheres given radius and center of 0,0,0
	MultiSphere(class Geometry &geom, class Textures &tex,
		   float radius, unsigned int num_spheres);
};

#endif
