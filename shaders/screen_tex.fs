#version 420 core

layout(binding = 0) uniform sampler2D sampler;

smooth in vec2 uv_vs;

layout(location = 0) out vec4 frag_color;

void main()
{
  frag_color = texture(sampler, uv_vs);
}
