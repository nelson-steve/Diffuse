#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inColor;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec2 outUV;
layout (location = 3) out vec3 outViewVec;
layout (location = 4) out vec3 outLightVec;

void main() 
{
    vec3 lightPos = vec3(0.0, 5.0, 0.0);
	outNormal = inNormal;
	outColor = inColor;
	outUV = inUV;
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPos.xyz, 1.0);
	
	vec4 pos = ubo.view * vec4(inPos, 1.0);
	outNormal = mat3(ubo.view) * inNormal;
	vec3 lPos = mat3(ubo.view) * lightPos.xyz;
	outLightVec = lightPos.xyz - pos.xyz;
	outViewVec = (vec3(ubo.view)).xyz - pos.xyz;	
}