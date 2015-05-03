#version 420 core 

in vec3 obj_pos; // object space vertex coordinates.
in vec3 yuv;
out vec3 yuv_vs;

void main(void) {
  yuv_vs = yuv;
  gl_Position = vec4(obj_pos, 1.0);
}
