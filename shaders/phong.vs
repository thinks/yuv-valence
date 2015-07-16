#version 420 core 

in vec3 obj_pos; // object space vertex coordinates.

void main(void) {
  gl_Position = vec4(obj_pos, 1.0);
}
