// fragment shader for simple terrain application
#version 400 core


// must match value in Geometry.hpp
#define MAX_DRAWS 100

// must match value in Image.hpp
#define MAX_TEXTURE_ARRAYS 8


// per-frame data using standard layout, matching C++
layout(std140) uniform FrameData { // per-frame data
	mat4 viewMatrix, viewInverse;
	mat4 projectionMatrix, projectionInverse;
};

// per-model data, components 4-float aligned
struct ModelDataStruct {
	mat4 matrix, inverse;		// model to view transform
	vec2 KdMap;					// diffuse texture descriptor
	vec2 KsMap;					// specular texture descriptor
	vec4 Kd;					// diffuse color multiplier
	vec3 Ks;					// specular at normal incidence
	float Ns;					// shininess
};
layout(std140) uniform ModelData {
	ModelDataStruct model[MAX_DRAWS];
};

// each textures[i] is a different size, containing all textures of that size
uniform sampler2DArray tex2d[MAX_TEXTURE_ARRAYS];

uniform float vertex_colors[MAX_DRAWS*3];

// input from vertex shader
in vec4 vPosition;
in vec3 vNormal;
in vec2 vTexcoord;
in float vID;
in vec3 vColor;

// output to frame buffer
out vec4 fragColor;

const float PI = 3.14159265359;


void main() {
	int id = int(vID + 0.5);
	
	fragColor = vec4(vColor, 0);
}
