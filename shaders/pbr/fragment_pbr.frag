#version 450

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inViewVec;
layout (location = 4) in vec3 inLightVec;

layout (binding = 1) uniform sampler2D albedoMap;
layout (binding = 2) uniform sampler2D normalMap;
layout (binding = 3) uniform sampler2D aoMap;
layout (binding = 4) uniform sampler2D metallicMap;
layout (binding = 5) uniform sampler2D roughnessMap;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	vec3 N = normalize(inNormal);
	vec3 V = normalize(ubo.camPos - inWorldPos);

	float roughness = material.roughness;

	// Add striped pattern to roughness based on vertex position
#ifdef ROUGHNESS_PATTERN
	roughness = max(roughness, step(fract(inWorldPos.y * 2.02), 0.5));
#endif

	// Specular contribution
	vec3 Lo = vec3(0.0);
	for (int i = 0; i < uboParams.lights.length(); i++) {
		vec3 L = normalize(uboParams.lights[i].xyz - inWorldPos);
		Lo += BRDF(L, V, N, material.metallic, roughness);
	};

	// Combine with ambient
	vec3 color = materialcolor() * 0.02;
	color += Lo;

	// Gamma correct
	color = pow(color, vec3(0.4545));

	outColor = vec4(color, 1.0);
}