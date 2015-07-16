#version 420 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout(std140) uniform Camera {
  mat4 view_from_world;
  mat4 clip_from_view;
};

layout(std140) uniform Model {
  mat4 world_from_obj;
  mat4 world_from_obj_normal;
};

flat out vec3 world_normal;

void main(void)
{
  vec3 obj_pos0 = gl_in[0].gl_Position.xyz;
  vec3 obj_pos1 = gl_in[1].gl_Position.xyz;
  vec3 obj_pos2 = gl_in[2].gl_Position.xyz;

  // Per-face normal. Same for all vertices of the emitted triangle.
  vec3 obj_normal = cross(obj_pos0 - obj_pos1, obj_pos2 - obj_pos1);
  world_normal = normalize(mat3(world_from_obj_normal) * obj_normal);

  mat4 clip_from_obj = clip_from_view * view_from_world * world_from_obj;

  int i;
  for (i = 0; i < gl_in.length(); ++i) {
    vec4 clip_pos = clip_from_obj * gl_in[i].gl_Position;
    gl_Position = clip_pos;
    EmitVertex();	
  }
  EndPrimitive();
}
