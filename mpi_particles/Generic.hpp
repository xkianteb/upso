// add cylinder object to draw list
#ifndef Generic_hpp
#define Generic_hpp
#include <map>
#include "Vec.hpp"

 
struct ObjInput{
	Vec<9> *faces;
	unsigned int num_faces;
	bool normals_present;
	char *mat_name;
};

struct Materials{
	Vec<3> ka;
	Vec<3> kd;
	Vec<3> ks;
	float tr;
	float illum;
	float ns;
	char *file_path;
	char *name;
};

struct Generic {
	unsigned int drawID;

	// create cylinder given radius in x/y, with given height in z
	// at location given by base
	Generic(class Geometry &geom, class Textures &tex, struct ObjInput input, Vec<3> *vertices, unsigned int num_vertices, Vec<2> *texture_coords, unsigned int num_texture_coords, Vec<3> *normal_lines, unsigned int num_normal_lines, Vec<3> base, float scalar, bool verbose, std::map<char*, struct Materials> material_lookup, char *base_path);
            //Vec<3> *vertices, unsigned int num_vertices, Vec<2> *texture_coords, unsigned int num_texture_coords, Vec<3> *normal_lines, unsigned int num_normal_lines, Vec<9> *faces, unsigned int num_faces, bool normals_present, Vec<3> base, float scalar);
};

#endif
