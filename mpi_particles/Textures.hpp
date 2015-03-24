// Track images and OpenGL textures
#ifndef Texture_hpp
#define Texture_hpp

#include "Image.hpp"
#include "Vec.hpp"
#include <string>

////////////////////
// Library of active textures
class Textures {
public:
	enum { // enum also provides scoped integer constants
		// GL version less than 4.1 likes fixed size arrays.
		// If you change this, update it in any shaders too.
		MAX_TEXTURE_ARRAYS=8,	// max different texture sizes

		MAX_ARRAY_LAYERS=8		// max textures of any one size
	};

	unsigned int numArrays;
	unsigned int numLayers[MAX_TEXTURE_ARRAYS];
	unsigned int textureIDs[MAX_TEXTURE_ARRAYS];
	Image lib[MAX_TEXTURE_ARRAYS][MAX_ARRAY_LAYERS];

// public methods
public:
	Textures();					// initialize as empty
	~Textures();					// gets rid of images & textures

	// find or load texture, return array and layer numbers
	Vec<2> findOrLoad(const std::string filename);
};

#endif
