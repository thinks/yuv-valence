#version 420 core 

in vec3 position; // object space vertex coordinates.
in vec2 tex2;
out vec2 tex2_vs;

void main(void) {
  tex2_vs = tex2;
  gl_Position = vec4(position, 1.0);
}
