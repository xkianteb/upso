// shader loading
#ifndef Shader_hpp
#define Shader_hpp

// know your Standard Template Library:
// list = doubly linked list
//	 not contiguous, but constant-time element insert and delete
#include <list>

// info we need to load a single shader
struct ShaderInfo {
	unsigned int id;			// shader object ID
	const char *file;			// file to load into this object
	ShaderInfo(unsigned int _id, const char *_file) {id=_id; file=_file;}
	ShaderInfo() {id=0; file=0;}
};

typedef std::list<ShaderInfo> ShaderList;

// load a set of shaders
// progID is the program object
// components is a list of shader components to link
// return false on compile error
bool loadShaders(unsigned int progID, const ShaderList &components);

#endif
