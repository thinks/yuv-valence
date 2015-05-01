#ifndef LIGHT_HPP_INCLUDED
#define LIGHT_HPP_INCLUDED

#include "Types.hpp"

class Light {
public:
  struct UniformData {
    GLfloat ambientColor[4];
    GLfloat diffuseColor[4];
    GLfloat specularColor[4];
    GLfloat position[4];
  };

  Light() {
    _data.ambientColor[0] = 1.f;
    _data.ambientColor[1] = 1.f;
    _data.ambientColor[2] = 1.f;
    _data.ambientColor[3] = 1.f;
    _data.diffuseColor[0] = 1.f;
    _data.diffuseColor[1] = 1.f;
    _data.diffuseColor[2] = 1.f;
    _data.diffuseColor[3] = 1.f;
    _data.specularColor[0] = 1.f;
    _data.specularColor[1] = 1.f;
    _data.specularColor[2] = 1.f;
    _data.specularColor[3] = 1.f;
    _data.position[0] = 0.f;
    _data.position[1] = 0.f;
    _data.position[2] = 0.f;
    _data.position[3] = 1.f;
  }

  UniformData const& uniformData() const {
    return _data;
  }

private:
  UniformData _data;
};

#endif // LIGHT_HPP_INCLUDED
