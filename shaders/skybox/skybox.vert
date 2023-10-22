#version 450

layout(location = 0) in vec3 inPosition;

layout(binding = 0) uniform UniformBufferObect {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout (location = 0) out vec3 outUVW;

void main() {
    outUVW = inPosition;
    gl_Position = ubo.model * ubo.proj * vec4(inPosition, 1.0);
}