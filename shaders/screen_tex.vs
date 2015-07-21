#version 420 core

layout(std140) uniform Camera
{
  mat4 view_from_world;
  mat4 clip_from_view;
};

layout(std140) uniform Model
{
  mat4 world_from_obj;
};

in vec3 obj_pos; // Object space vertex coordinates.
in vec2 uv; // Texture coordinates.
smooth out vec2 uv_vs;

void main()
{
  uv_vs = uv;
  vec4 world_pos = world_from_obj * vec4(obj_pos, 1.0);
  vec4 view_pos = view_from_world * world_pos;
  vec4 clip_pos = clip_from_view * view_pos;
  gl_Position = clip_pos;
}
