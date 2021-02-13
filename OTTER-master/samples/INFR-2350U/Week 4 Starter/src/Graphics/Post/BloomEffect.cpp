#include "BloomEffect.h"

void BloomEffect::Init(unsigned width, unsigned height)
{
	int index = int(_buffers.size());

	_buffers.push_back(new Framebuffer());
	_buffers[index]->AddColorTarget(GL_RGBA8);
	_buffers[index]->AddDepthTarget();
	_buffers[index]->Init(width, height);
	index++;

	_buffers.push_back(new Framebuffer());
	_buffers[index]->AddColorTarget(GL_RGBA8);
	_buffers[index]->AddDepthTarget();
	_buffers[index]->Init(unsigned(width / _downscale), unsigned(height / _downscale));
	index++;

	_buffers.push_back(new Framebuffer());
	_buffers[index]->AddColorTarget(GL_RGBA8);
	_buffers[index]->AddDepthTarget();
	_buffers[index]->Init(unsigned(width / _downscale), unsigned(height / _downscale));
	index++;

	_buffers.push_back(new Framebuffer());
	_buffers[index]->AddColorTarget(GL_RGBA8);
	_buffers[index]->AddDepthTarget();
	_buffers[index]->Init(width, height);

	//Load in the shaders
	int index2 = int(_shaders.size());
	_shaders.push_back(Shader::Create());
	_shaders[index2]->LoadShaderPartFromFile("shaders/passthrough_vert.glsl", GL_VERTEX_SHADER);
	_shaders[index2]->LoadShaderPartFromFile("shaders/Post/PassThrough_frag.glsl", GL_FRAGMENT_SHADER);
}

void BloomEffect::ApplyEffect(PostEffect* buffer)
{
}

void BloomEffect::Reshape(unsigned width, unsigned height)
{
}

float BloomEffect::GetDownscale() const
{
	return 0.0f;
}

float BloomEffect::GetThreshold() const
{
	return 0.0f;
}

unsigned BloomEffect::GetPasses() const
{
	return 0;
}

void BloomEffect::SetDownscale(float downscale)
{
}

void BloomEffect::SetThreshold(float threshold)
{
}

void BloomEffect::SetPasses(unsigned passes)
{
}