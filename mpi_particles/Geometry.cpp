// maintain graphics draw data

#include "Geometry.hpp"
#include "Textures.hpp"
#include "Vec.inl"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <assert.h>
#include <stdio.h>
#include <stddef.h>

#ifdef _WIN32
// don't complain if we use standard IO functions instead of windows-only
#pragma warning( disable: 4996 )
#endif

// create necessary buffers
Geometry::Geometry() {
	numDraws = 0;
	glGenBuffers(NUM_BUFFERS, bufferIDs);
	glGenVertexArrays(1, &varrayID);

	// load shaders: more complex code would have a list of shaders with lists 
	// of draws for each want to render everything with each shader in a batch
	shaderParts.push_back(ShaderInfo(glCreateShader(GL_VERTEX_SHADER),
									 "tex.vert"));
	shaderParts.push_back(ShaderInfo(glCreateShader(GL_FRAGMENT_SHADER),
									 "tex.frag"));
	shaderID = glCreateProgram();
	updateShaders();
}

// delete OpenGL objects
Geometry::~Geometry() {
	for (ShaderList::iterator ci = shaderParts.begin(); 
		 ci != shaderParts.end(); ++ci) {
		glDeleteShader(ci->id);
	}
	glDeleteProgram(shaderID);

	glDeleteBuffers(NUM_BUFFERS, bufferIDs);
	glDeleteVertexArrays(1, &varrayID);
}

// helper to (re)attach OpenGL buffer to a shader variable by name
static void bindBuffer(
	unsigned int shaderID, const char *paramName, unsigned int bufferID, 
	unsigned int dimensions, unsigned int stride, unsigned int offset)
{
	GLint attrib = glGetAttribLocation(shaderID, paramName);
	glBindBuffer(GL_ARRAY_BUFFER, bufferID);
	glVertexAttribPointer(attrib, dimensions, GL_FLOAT, GL_FALSE, stride, 
						  (char*)0+offset);
	glEnableVertexAttribArray(attrib);
}

// load (or replace) shaders
void Geometry::updateShaders() {
	loadShaders(shaderID, shaderParts);
	glUseProgram(shaderID);

	// map shader name to texture to glActiveTexture number
	for(int i=0; i<Textures::MAX_TEXTURE_ARRAYS; ++i) {
		char texname[16];
		sprintf(texname, "tex2d[%d]", i);
		glUniform1i(glGetUniformLocation(shaderID, texname), i);
	}

	// re-connect attribute arrays
	glBindVertexArray(varrayID);
	bindBuffer(shaderID, "position", bufferIDs[VERT_BUFFER],   3, sizeof(Vertex), offsetof(Vertex, pos));
	bindBuffer(shaderID, "normal",	 bufferIDs[VERT_BUFFER],   3, sizeof(Vertex), offsetof(Vertex, norm));
	bindBuffer(shaderID, "uv",		 bufferIDs[VERT_BUFFER],   2, sizeof(Vertex), offsetof(Vertex, uv));
	bindBuffer(shaderID, "drawID",	 bufferIDs[DRAWID_BUFFER], 1, sizeof(float),  0);

	// (re)connect uniform blocks
	glUniformBlockBinding(shaderID, glGetUniformBlockIndex(shaderID,"FrameData"), FRAME_UNIFORMS);
	glUniformBlockBinding(shaderID, glGetUniformBlockIndex(shaderID,"ModelData"), MODEL_UNIFORMS);
}

// reserve space for a new draw, returning draw ID
unsigned int Geometry::addDraw(unsigned int vtxCount, unsigned int idxCount) {
	// update counts
	unsigned int vtxStart = vert.size(),  vertTotal = vtxStart + vtxCount;
	unsigned int idxStart = tris.size(), indexTotal = idxStart + idxCount;
	unsigned int id = numDraws++;
	
	assert(numDraws < MAX_DRAWS);

	// resize arrays
	vert.resize(vertTotal);
	drawID.resize(vertTotal);
	tris.resize(indexTotal);

	// fill in drawID for each new vertex
	for(unsigned int i=vtxStart;  i != vertTotal;  ++i)
		drawID[i] = float(id);

	// add to draw lists
	baseVertex[id] = vtxStart;
	indexCount[id] = idxCount;
	baseIndex[id] = idxStart;
	baseIndexOffset[id] = (Index*)0 + idxStart;
	return id;					// return ID
}

// get pointer to start of vertex array for specific draw
Geometry::Vertex *Geometry::getVertices(unsigned int drawID) {
	assert(drawID < numDraws);
	return &vert[baseVertex[drawID]];
}

// get pointer to start of vertex array for specific draw
Geometry::Index *Geometry::getIndices(unsigned int drawID) {
	assert(drawID < numDraws);
	return &tris[baseIndex[drawID]];
}

// get uniform shader data for model
Geometry::ModelUniforms &Geometry::getModelUniforms(unsigned int drawID) {
	assert(drawID < numDraws);
	return modelUniforms[drawID];
}

// update vertex and index data after last addDraw
void Geometry::finalizeDrawData() {
	// update draw data after it's all filled in
	unsigned int vertTotal = vert.size(), indexTotal = tris.size();
	glBindVertexArray(varrayID);

	// load data into each buffer: GL_STATIC_DRAW since we won't change it
	glBindBuffer(GL_ARRAY_BUFFER, bufferIDs[VERT_BUFFER]);
	glBufferData(GL_ARRAY_BUFFER, vertTotal*sizeof(Vertex), &vert[0],
				 GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, bufferIDs[DRAWID_BUFFER]);
	glBufferData(GL_ARRAY_BUFFER, vertTotal*sizeof(float),	&drawID[0],
				 GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferIDs[INDEX_BUFFER]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexTotal*sizeof(Index), &tris[0],
				 GL_STATIC_DRAW);

	// per-model uniform data, this one is GL_DYNAMIC_DRAW since it changes
	glBindBuffer(GL_UNIFORM_BUFFER, bufferIDs[MODEL_UNIFORM_BUFFER]);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(modelUniforms), &modelUniforms[0],
				 GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, MODEL_UNIFORMS,
					 bufferIDs[MODEL_UNIFORM_BUFFER]);
}

// draw current geometry
void Geometry::draw() const {
	// update per-model uniform data
	glBindBuffer(GL_UNIFORM_BUFFER, bufferIDs[MODEL_UNIFORM_BUFFER]);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(modelUniforms),
					&modelUniforms[0]);

	// draw everything
	glMultiDrawElementsBaseVertex(
		GL_TRIANGLES, &indexCount[0], GL_UNSIGNED_INT,
		(GLvoid**)&baseIndexOffset[0], numDraws, &baseVertex[0]);
}
