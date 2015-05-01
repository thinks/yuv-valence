#version 420 core 

layout(std140) uniform Material {
  vec4 materialFrontAmbientColor;
  vec4 materialBackAmbientColor;
  vec4 materialFrontDiffuseColor;
  vec4 materialBackDiffuseColor;
  vec4 materialFrontSpecularColor;
  vec4 materialBackSpecularColor;
};

layout(std140) uniform LightColor {
  vec4 lightAmbientColor;
  vec4 lightDiffuseColor;
  vec4 lightSpecularColor;
};

//smooth in vec3 viewDirection;
//smooth in vec3 lightDirection;
flat in vec3 viewDirection;
flat in vec3 lightDirection;
flat in vec3 viewNormal;
smooth in vec4 tex2_gs;

out vec4 fragColor;

void main(void) {
  vec3 fragNormal = normalize(viewNormal);
  vec3 fragLightDirection = normalize(lightDirection);
  vec3 fragViewDirection = normalize(viewDirection);
  vec4 ambientColor;
  vec4 diffuseColor;
  vec4 specularColor;
  float shininess;
 
  if (gl_FrontFacing) {
    ambientColor = materialFrontAmbientColor;
    diffuseColor = materialFrontDiffuseColor;
    specularColor = materialFrontSpecularColor;
    shininess = 16.0;
  }
  else {
    ambientColor = materialBackAmbientColor;
    diffuseColor = materialBackDiffuseColor;
    specularColor = materialBackSpecularColor;
    shininess = 16.0;
    fragNormal = -fragNormal; // Flip normal!
  }


  float diffuse = max(dot(fragNormal, fragLightDirection), 0.0);

  vec3 reflectedNormal = reflect(-fragLightDirection, fragNormal);
  float specular = 
    pow(max(dot(reflectedNormal, fragViewDirection), 0.0), shininess);

  mat3 rgb_from_yuv = mat3(
    1.0,      1.0,     1.0,     // Column 0
    0.0,     -0.21482, 2.12798,
    1.28033, -0.38059, 0.0);

  vec4 rgb = vec4(rgb_from_yuv * vec3(0.5, tex2_gs.s, tex2_gs.t), 1.0);
  //vec4 rgb = vec4(1.0, 1.0, 1.0, 1.0);

  fragColor = 
    rgb * (
    lightAmbientColor*ambientColor + 
    diffuse*(lightDiffuseColor*diffuseColor) + 
    specular*(lightSpecularColor*specularColor));
}
