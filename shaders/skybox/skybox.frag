#version 450

layout (location = 0) in vec3 inUVW;
layout(location = 0) out vec4 outColor;

layout (binding = 1) uniform samplerCube env_texture;

void main() {
    vec3 color = textureLod(env_texture, inUVW, 1.0).rgb;
    outColor = vec4(color, 1.0);
}