#version 450

//layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec4 outColor;

layout (binding = 0) uniform samplerCube env_texture;

void main() {
    vec3 env_vector = vec3(1.0, 1.0, 1.0);
    outColor = textureLod(env_texture, env_vector, 0);
}