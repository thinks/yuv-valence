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

out vec4 fragColor;

void main(void) {
  vec3 frag_normal = normalize(world_normal);
  vec3 frag_light_direction = normalize(light_direction.xyz);

  float diffuse = max(dot(frag_normal, frag_light_direction), 0.0);

  //fragColor =
  //  diffuse * (light_diffuse_color * material_front_diffuse_color);
  gl_Fragdata[0] = diffuse * (light_diffuse_color * material_front_diffuse_color);
  //fragColor = vec4(abs(fragNormal.xyz), 1.0);
  //fragColor = vec4(vec3(abs(fragNormal.x), abs(fragNormal.y), 0.0), 1.0);
  //fragColor = vec4(vec3(0.0, abs(fragNormal.y), 0.0), 1.0);
}
