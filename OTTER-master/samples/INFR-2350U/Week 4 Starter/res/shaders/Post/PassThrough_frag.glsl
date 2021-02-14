#version 420

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 frag_color;

layout (binding = 0) uniform sampler2D s_Tex;

void main() 
{
	vec4 source = texture(s_Tex, inUV);

	frag_color.rgb = source.rgb;
	frag_color.a = source.a;
}