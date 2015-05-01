#ifndef MATERIAL_HPP_INCLUDED
#define MATERIAL_HPP_INCLUDED

#include "Types.hpp"

class Material {
public:
  struct UniformData {
    vec4f frontAmbientColor;
    vec4f backAmbientColor;
    vec4f frontDiffuseColor;
    vec4f backDiffuseColor;
    vec4f frontSpecularColor;
    vec4f backSpecularColor;
  };

  Material(vec4f const& frontAmbientColor = vec4f(.2f, .2f, .2f, 1.f),
           vec4f const& backAmbientColor = vec4f(.2f, .2f, .2f, 1.f),
           vec4f const& frontDiffuseColor = vec4f(.4f, .5f, .8f, 1.f),
           vec4f const& backDiffuseColor = vec4f(.8f, .5f, .4f, 1.f),
           vec4f const& frontSpecularColor = vec4f(.4f, .4f, .4f, 1.f),
           vec4f const& backSpecularColor = vec4f(.4f, .4f, .4f, 1.f)) {
    _data.frontAmbientColor = frontAmbientColor;
    _data.backAmbientColor = backAmbientColor;
    _data.frontDiffuseColor = frontDiffuseColor;
    _data.backDiffuseColor = backDiffuseColor;
    _data.frontSpecularColor = frontSpecularColor;
    _data.backSpecularColor = backSpecularColor;
  }

  UniformData const&
  uniformData() const {
    return _data;
  }

private: // Member variables.
  UniformData _data;
};

#endif // MATERIAL_HPP_INCLUDED
