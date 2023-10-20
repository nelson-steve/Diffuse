#version 450

layout(location = 0) in vec3 inPosition;

//layout(binding = 0) uniform UniformBufferObect {
//    mat4 proj;
//} ubo;

//layout(location = 0) out vec3 outPosition;

void main() {
    gl_Position = vec4(inPosition, 1.0);
    //outPosition = inPosition;
}