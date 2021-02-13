#version 420

layout(location = 0) in vec2 inUV;

out vec4 frag_color;

layout(binding = 0) uniform sampler2D s_Tex; 

uniform float u_Threshold;

void main()
{
	vec4 color = texture(s_Tex, inUV);

	float luminence = (color.r + color.g + color.b) / 3.0;

	if (luminence > u_Threshold)
	{
		frag_color = color;
	}
	else
	{
		frag_color = vec4(0.0, 0.0, 0.0, 1.0);
	}
}