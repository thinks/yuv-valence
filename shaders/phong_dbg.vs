#version 420 core 

in vec3 position; // object space vertex coordinates.
in vec3 yuv;
out vec2 yuv_vs;

void main(void) {
  yuv_vs = yuv;
  gl_Position = vec4(position, 1.0);
}
