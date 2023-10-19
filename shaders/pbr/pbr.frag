#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

layout (binding = 1) uniform sampler2D baseColorTexture;
layout (binding = 2) uniform sampler2D metallicRoughnessTexture;
layout (binding = 3) uniform sampler2D normalTexture;
layout (binding = 4) uniform sampler2D occlusionTexture;
layout (binding = 5) uniform sampler2D emissiveTexture;

void main() {
    outColor = vec4(fragColor, 1.0);
}