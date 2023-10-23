#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 uv0;
layout(location = 2) in vec3 local_pos;

layout(location = 0) out vec4 outColor;

layout (binding = 1) uniform sampler2D baseColorTexture;
layout (binding = 2) uniform sampler2D metallicRoughnessTexture;
layout (binding = 3) uniform sampler2D normalTexture;
layout (binding = 4) uniform sampler2D occlusionTexture;
layout (binding = 5) uniform sampler2D emissiveTexture;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main() {
    //vec3 brdf = texture(baseColorTexture, vec2(1.0, 1.0), 1.0).rgb;
    vec2 uv = SampleSphericalMap(normalize(local_pos));
    outColor = texture(baseColorTexture, uv0);
    //outColor = vec4(brdf, uv0);
}