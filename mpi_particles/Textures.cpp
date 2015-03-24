// Generic image data

#include "Textures.hpp"
#include "Image.hpp"
#include "Vec.inl"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <assert.h>

// translate number of channels to texture format
static GLenum format[] = {
	GL_NONE,					// no 0-channel textures
	GL_LUMINANCE,				// RGB=value0, A=1
	GL_LUMINANCE_ALPHA,			// RGB=value0, A=value1
	GL_RGB,						// R=value0, G=value1, B=value2, A=1
	GL_RGBA						// R=value0, G=value1, B=value2, A=value3
};

// translate number of channels to storage format
static GLenum storage[] = {
	GL_NONE,					// no 0-channel textures
	GL_LUMINANCE8,				// 8-bit per channel luminance texture
	GL_LUMINANCE8_ALPHA8,		// 8-bit per channel luminance-alpha texture
	GL_RGB8,					// 8-bit per channel RGB texture
	GL_RGBA8					// 8-bit per channel RGBA texture
};

// initialize as empty
Textures::Textures() {
	numArrays=0;
	glGenTextures(MAX_TEXTURE_ARRAYS, textureIDs); // create texture arrays
}

// make sure to free allocated images and textures
Textures::~Textures() {
	glDeleteTextures(MAX_TEXTURE_ARRAYS, textureIDs);
}

// find texture if already loaded; load it if not
// return texture array and layer
Vec<2> Textures::findOrLoad(const std::string filename) {
	// do we already have this image?
	unsigned int array, layer;
	for(array=0; array != numArrays; ++array)
		for(layer=0; layer != numLayers[array]; ++layer)
			if (filename == lib[array][layer].name)
				return Vec2(float(array), float(layer));

	// read PPM header to find out size
	Image im;
	FILE *fp = PAMopen(filename.c_str(), im);

	// do we already have a texture array for this size?
	for(array=0; array != numArrays; ++array) {
		if (lib[array][0].width	 == im.width  && 
			lib[array][0].height == im.height && 
			lib[array][0].depth	 == im.depth)
			break;
	}

	// increse MAX_TEXTURE_ARRAYS if this assert trips
	assert(array != MAX_TEXTURE_ARRAYS);

	glActiveTexture(GL_TEXTURE0 + array);
	glBindTexture(GL_TEXTURE_2D_ARRAY, textureIDs[array]);

	// new size, need new texture array
	if (array == numArrays) {
		numArrays++;
		numLayers[array] = 0;

		// how many mip levels?
		unsigned int mips = 0;
		for(int w=im.width, h=im.height; w != 0 && h != 0; w>>=1, h>>=1, ++mips) {}

		// allocate texture
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, 
						GL_LINEAR_MIPMAP_LINEAR);
		glTexStorage3D(GL_TEXTURE_2D_ARRAY, mips, storage[im.depth], 
					   im.width, im.height, MAX_ARRAY_LAYERS);
	}

	// read into new layer of this texture array
	layer = numLayers[array]++;
	assert(layer != MAX_ARRAY_LAYERS);
	lib[array][layer] = im;
	PAMread(fp, lib[array][layer]);

	// load into texture
	glTexSubImage3D(
		GL_TEXTURE_2D_ARRAY, 0, 0, 0, layer, im.width, im.height, 1,
		format[im.depth], GL_UNSIGNED_BYTE, lib[array][layer].data);
	glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

	return Vec2(float(array), float(layer));
}
