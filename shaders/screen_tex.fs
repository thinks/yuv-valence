#version 420 core

uniform sampler2D sampler;

smooth in vec2 uv_vs;

layout(location = 0) out vec4 frag_color;

void main()
{
  //frag_color = vec4(vec3(uv_vs, 0.0), 1.0);
  frag_color = texture(sampler, uv_vs);
}
