#version 420 core 

layout(std140) uniform Material {
  vec4 material_front_diffuse_color;
};

layout(std140) uniform LightColor {
  vec4 light_diffuse_color;
};

layout(std140) uniform LightDirection {
  vec4 light_direction;
};

flat in vec3 world_normal;
smooth in vec3 yuv_gs;

out vec4 frag_color;

void main(void)
{
  mat3 rgb_from_yuv = mat3(
    1.0,      1.0,     1.0,     // Column 0
    0.0,     -0.21482, 2.12798,
    1.28033, -0.38059, 0.0);
  vec4 rgb = vec4(rgb_from_yuv * yuv_gs, 1.0);

  vec3 frag_normal = normalize(world_normal);
  vec3 frag_light_direction = normalize(-light_direction.xyz);

  float diffuse = max(dot(frag_normal, frag_light_direction), 0.0);

  frag_color = rgb *
    (diffuse * (light_diffuse_color * material_front_diffuse_color));
}
