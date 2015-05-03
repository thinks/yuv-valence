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

in vec3 yuv_vs[];

flat out vec3 world_normal;
smooth out vec3 yuv_gs;

void main(void) 
{
  vec4 obj_pos0 = gl_in[0].gl_Position;
  vec4 obj_pos1 = gl_in[1].gl_Position;
  vec4 obj_pos2 = gl_in[2].gl_Position;

  // Per-face normal. Same for all vertices of the emitted triangle.
  vec3 obj_normal = cross(obj_pos2.xyz - obj_pos1.xyz,
                          obj_pos0.xyz - obj_pos1.xyz);
  world_normal = normalize(mat3(world_from_obj_normal) * obj_normal);

  mat4 clip_from_obj = clip_from_view * view_from_world * world_from_obj;

  yuv_gs = yuv_vs[0];
  gl_Position = clip_from_obj * obj_pos0;
  EmitVertex();

  yuv_gs = yuv_vs[1];
  gl_Position = clip_from_obj * obj_pos1;
  EmitVertex();

  yuv_gs = yuv_vs[2];
  gl_Position = clip_from_obj * obj_pos2;
  EmitVertex();

  EndPrimitive();
}
