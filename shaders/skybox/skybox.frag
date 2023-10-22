#version 450

layout (location = 0) in vec3 inUVW;
layout(location = 0) out vec4 outColor;

layout (binding = 1) uniform samplerCube env_texture;

void main() {
    vec3 env_vector = normalize(inUVW);
    outColor = textureLod(env_texture, env_vector, 0);
}