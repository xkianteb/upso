// fragment shader for simple terrain application
#version 400 core


// must match value in Geometry.hpp
#define MAX_DRAWS 351

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

// input from vertex shader
in vec4 vPosition;
in vec3 vNormal;
in vec2 vTexcoord;
in float vID;

// output to frame buffer
out vec4 fragColor;

const float PI = 3.14159265359;


void main() {
	int id = int(vID + 0.5);
	
	
	fragColor = vec4(0);
	return;

	// lighting vectors
	vec3 L = normalize(vec3(-1,1,1));	 // light direction
	vec3 V = normalize(-vPosition.xyz); // view direction (assuming eye at 0)
	vec3 H = normalize(V + L);			 // half vector

	// re-normalize normal
	vec3 N = normalize(vNormal);

	// lighting dot products
	float N_L = max(0., dot(N,L));
	float N_V = max(0., dot(N,V));
	float N_H = max(0., dot(N,H));
	float V_H = max(0., dot(V,H));


	// base diffuse color
	vec4 color = model[id].Kd;
	if (model[id].KdMap.x >= 0)
		color *= texture(tex2d[int(model[id].KdMap.x)], 
						 vec3(vTexcoord, model[id].KdMap.y));
	if (color.a == 0) discard;	// cut out transparent stencils

	// base specular color
	vec3 Ks = model[id].Ks;
	if (model[id].KsMap.x >= 0)
		Ks *= texture(tex2d[int(model[id].KsMap.x)], 
					  vec3(vTexcoord, model[id].KsMap.y)).rgb;

	// microfacet specular terms:
	// Blinn-Phong distribution, Schlick/Smith masking, Schlick Fresnel
	float var = 2./(model[id].Ns + 2.);	// normal variance
	float D = pow(max(0., N_H), model[id].Ns) / var;
	float G = sqrt(var/PI);
	G = 1. / ((N_L * (1 - G) + G) * (N_V * (1 - G) + G));
	vec3 F = Ks + (1 - Ks) * pow(1 - V_H, 5.);
	color.rgb = mix(color.rgb*(1-F), vec3(D*G), F) * N_L;

	// final color
	fragColor = color;
}
