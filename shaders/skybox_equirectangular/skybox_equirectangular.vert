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
    mat4 _view = mat4(mat3(ubo.view));
    gl_Position  = ubo.proj * _view * vec4(inPosition, 1.0);
}