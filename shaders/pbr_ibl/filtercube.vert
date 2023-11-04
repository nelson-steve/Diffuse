#version 450

layout (location = 0) in vec3 in_position;

layout(push_constant) uniform PushConsts {
	layout (offset = 0) mat4 mvp;
} pushConsts;

layout (location = 0) out vec3 out_uv;

void main() 
{
	out_uv = in_position;
	gl_Position = pushConsts.mvp * vec4(in_position.xyz, 1.0);
}
