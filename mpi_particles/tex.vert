// vertex shader for simple terrain demo
#version 400 core


// must match value in Geometry.hpp
#define MAX_DRAWS 100


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

// per-vertex input
in vec3 position;
in vec3 normal;
in vec2 uv;
in float drawID;
in vec3 color;

// output to fragment shader
out vec4 vPosition;
out vec3 vNormal;
out vec2 vTexcoord;
out float vID;
out vec3 vColor;

void main() {
	// integer ID for which set of model data to use
	int id = int(drawID + 0.5);
	vID = id;

	// combined transform from model space to view space
	mat4 MV = viewMatrix * model[id].matrix;
	mat4 MVi = model[id].inverse * viewInverse;

	// transform position to view space and to projection space
	vPosition = MV * vec4(position, 1);
	gl_Position = projectionMatrix * vPosition;

	// per-vertex normal
	vNormal = normalize(normal * mat3(MVi));
	
	// just pass texture coordinates through
	vTexcoord = uv;
	
	vColor = color;
}
