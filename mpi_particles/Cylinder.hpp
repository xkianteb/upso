// add cylinder object to draw list
#ifndef Cylinder_hpp
#define Cylinder_hpp

#include "Vec.hpp"

struct Cylinder {
	unsigned int drawID;
 
	// create cylinder given radius in x/y, with given height in z
	// at location given by base
	Cylinder(class Geometry &geom, class Textures &tex, 
			 float radius, float height, Vec<3> base, float scalar);
};

#endif
