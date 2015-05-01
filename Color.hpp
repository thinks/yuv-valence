#ifndef COLOR_HPP_INCLUDED
#define COLOR_HPP_INCLUDED

class Color {
public:
  struct UniformData {
    GLfloat color[4];
  };

  Color(GLfloat const r, GLfloat const g, GLfloat const b, GLfloat const a) {
    _data.color[0] = r;
    _data.color[1] = g;
    _data.color[2] = b;
    _data.color[3] = a;
  }

  UniformData const& uniformData() const {
    return _data;
  }

private:
  UniformData _data;
};

#endif // COLOR_HPP_INCLUDED
