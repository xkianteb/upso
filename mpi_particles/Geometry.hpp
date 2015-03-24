// collected graphics draw data
#ifndef Geometry_hpp
#define Geometry_hpp

#include "Vec.hpp"
#include "Xform.hpp"
#include "Shader.hpp"

// know your Standard Template Library:
// vector = dynamically sized array
//	  guaranteed to be contiguous in memory (good for GPU)
//	  but resizing may allocate a bigger array and copy it over
//	  existing pointers to elements will be invalid after resize
#include <vector>

// GL version less than 4.1 likes fixed size arrays.
// If you change this, update it in any shaders too.
#define MAX_DRAWS 351

class Geometry {
public:
	// shader "uniform" parameter blocks
	enum UniformBlocks {
		FRAME_UNIFORMS,	// data that changes once per frame (view, lights)
		MODEL_UNIFORMS,	// per-object data (transforms, parameters, etc.)
		NUM_UNIFORM_BLOCKS
	};

	// per frame shader data type
	struct FrameUniforms {
		Xform viewMats;			// world to view matrix & inverse
		Xform projMats;			// view to NDC matrix & inverse
	};

	// per draw shader data type (components should be 4-float aligned)
	struct ModelUniforms {
		Xform modelMats;		// object to view matrix & inverse
		Vec<2> KdMap;			// diffuse texture descriptor
		Vec<2> KsMap;			// specular texture descriptor
		Vec<4> Kd;				// diffuse color multiplier
		Vec<3> Ks;				// specular color at normal incidence
		float Ns;				// shininess
	};

	// per vertex data type
	struct Vertex {
		Vec<3> pos, norm;		// position, normal
		Vec<2> uv;				// u,v texture coordinates
	};
	typedef unsigned int Index; // triangle index type

private:
	// parameters for each draw
	unsigned int numDraws;		// number of models to draw
	int baseVertex[MAX_DRAWS];	// starting vertex for each draw
	int indexCount[MAX_DRAWS];	// number of indices for each draw
	Index baseIndex[MAX_DRAWS]; // starting index
	Index *baseIndexOffset[MAX_DRAWS]; // offset to starting index, as pointer
	ModelUniforms modelUniforms[MAX_DRAWS]; // per draw shader data

	std::vector<Vertex> vert;				// per vertex data
	std::vector<Index> tris;				// triangle index data

	// per-vertex, which draw it belongs to, used to index into ModelData
	// Some GL versions have built-in gl_DrawID for this
	std::vector<float> drawID;

	// shaders
	unsigned int shaderID;
	ShaderList shaderParts;

	// GL buffer object IDs
	enum Buffers {
		VERT_BUFFER, DRAWID_BUFFER, INDEX_BUFFER, MODEL_UNIFORM_BUFFER,
		NUM_BUFFERS};
	unsigned int bufferIDs[NUM_BUFFERS];
	unsigned int varrayID;

public: // public member functions
	// create and destroy
	Geometry();
	~Geometry();

	// load (or replace) shaders
	void updateShaders();

	// reserve space for a new draw, returning draw ID
	unsigned int addDraw(unsigned int vertCount, unsigned int indexCount);

	// return start of vertex and index array segments for a particular draw
	// only valid until next addDraw, since vectors could resize
	Vertex *getVertices(unsigned int drawID);
	Index *getIndices(unsigned int drawID);
	ModelUniforms &getModelUniforms(unsigned int drawID);

	// update vertex and index data after last addDraw
	void finalizeDrawData();

	// draw current geometry
	void draw() const;
};

#endif
