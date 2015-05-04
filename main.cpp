#include <array>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <nDjinn.hpp>
#include <thx.hpp>

using namespace std;
using namespace ndj;
using namespace thx;

GLFWwindow* win = nullptr;
int winWidth = 480;
int winHeight = 480;

unique_ptr<ShaderProgram> phong_yuv;
unique_ptr<VertexArray> phong_yuv_va;
unique_ptr<UniformBuffer> camera_ubo;
unique_ptr<UniformBuffer> light_color_ubo;
unique_ptr<UniformBuffer> light_direction_ubo;
unique_ptr<UniformBuffer> material_ubo;
unique_ptr<UniformBuffer> model_ubo;
unique_ptr<ArrayBuffer> obj_pos_vbo;
unique_ptr<ArrayBuffer> yuv_vbo;
unique_ptr<ElementArrayBuffer> tri_index_ibo;

typedef vec<2, GLfloat> Vec2f;
typedef vec<3, GLfloat> Vec3f;
typedef vec<3, GLushort> Vec3us;

GLfloat triangleArea(const Vec3f& v0, const Vec3f& v1, const Vec3f& v2) {
  return 0.5f * thx::mag(cross(v2 - v0, v2 - v1));
}

Vec3f triangleCenter(const Vec3f& v0, const Vec3f& v1, const Vec3f& v2) {
  return (1.0f / 3) * (v0 + v1 + v2);
}

struct Triangle {
  Triangle(const GLushort i, const GLushort j, const GLushort k)
    : i(i), j(j), k(k) {}
  GLushort i;
  GLushort j;
  GLushort k;
};

//! DOCS
void framebufferSizeCallback(GLFWwindow* win,
                             const int w, const int h)
{
  //cout << "framebufferSizeCallback" << endl;

  winWidth = w;
  winHeight = h;
  viewport(0, 0, w, h);
}

//! DOCS
void keyCallback(GLFWwindow* window,
                 int key, int scancode, int action, int mods)
{
  //cout << "keyCallback" << endl;
}

//! DOCS
void cursorPosCallback(GLFWwindow* win,
                       const double mx, const double my)
{
  //cout << "cursorPosCallback" << endl;
}

//! DOCS
void mouseButtonCallback(GLFWwindow* window,
                         int button, int action, int mods)
{
  //cout << "mouseButtonCallback" << endl;
}

//! DOCS
void scrollCallback(GLFWwindow* win, double xoffset, double yoffset)
{
  //cout << "scrollCallback" << endl;
}

//! DOCS
void bindUniformBuffer(ndj::ShaderProgram const& shaderProgram,
                       std::string const& uniformBlockName,
                       ndj::UniformBuffer const& ubo)
{
  UniformBlock const& uniformBlock =
    shaderProgram.activeUniformBlock(uniformBlockName);
  if (uniformBlock.size() != ubo.sizeInBytes()) {
    NDJINN_THROW("uniform block and uniform buffer size mismatch");
  }

  ubo.bindBase(uniformBlock.binding());
}

//! DOCS
void initGLFW(const int width, const int height)
{
  if (!glfwInit()) {
    throw runtime_error("GLFW init error");
  }

  glfwWindowHint(GLFW_SAMPLES, 4);

  win = glfwCreateWindow(width, height, "yuv-valence", nullptr, nullptr);
  if (win == nullptr) {
    throw runtime_error("GLFW Open Window error");
  }
  glfwMakeContextCurrent(win);
  glfwSetFramebufferSizeCallback(win, framebufferSizeCallback);
  glfwSetKeyCallback(win, keyCallback);
  glfwSetCursorPosCallback(win, cursorPosCallback);
  glfwSetMouseButtonCallback(win, mouseButtonCallback);
  glfwSetScrollCallback(win, scrollCallback);
}

//! DOCS
void initGLEW()
{
  try {
    glewExperimental = GL_TRUE;
    GLenum const glewErr = glewInit();
    if (GLEW_OK != glewErr) {
      // glewInit failed, something is seriously wrong.
      cerr << "GLEW init error: " << glewGetErrorString(glewErr) << endl;
      abort();
    }
    checkError("glewInit");
  }
  catch (exception const& ex) {
    cout << "Warning: " << ex.what() << endl;
  }
}

void initGL()
{
  clearColor(0.2f, 0.2f, 0.2f, 1.f);
  clearDepth(1.0);
  depthRange(0.0, 1.0);
  enable(GL_MULTISAMPLE);
  frontFace(GL_CCW);
  cullFace(GL_BACK);
  enable(GL_CULL_FACE);

  glfwGetFramebufferSize(win, &winWidth, &winHeight);
  viewport(0, 0, winWidth, winHeight);

  int glfwMajor = 0;
  int glfwMinor = 0;
  int glfwRev = 0;
  glfwGetVersion(&glfwMajor, &glfwMinor, &glfwRev);

  cout
    << "GLFW version: "
       << glfwMajor << "." << glfwMinor << "." << glfwRev << "\n"
    << "GLEW_VERSION: " << glewGetString(GLEW_VERSION) << "\n"
    << "GL_VERSION: " << getString(GL_VERSION) << "\n"
    << "GL_VENDOR: " << getString(GL_VENDOR) << "\n"
    << "GL_RENDERER: " << getString(GL_RENDERER) << "\n"
    << "GL_SHADING_LANGUAGE_VERSION: "
      << getString(GL_SHADING_LANGUAGE_VERSION) << "\n";

  GLint maxVertexAttribs = 0;
  getIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs);
  cout << "GL_MAX_VERTEX_ATTRIBS: " << maxVertexAttribs << "\n";
}

void buildShaderPrograms()
{
  phong_yuv.reset(new ShaderProgram(
    VertexShader(readShaderFile("shaders/phong_yuv.vs")),
    GeometryShader(readShaderFile("shaders/phong_yuv.gs")),
    FragmentShader(readShaderFile("shaders/phong_yuv.fs"))));
  phong_yuv->activeUniformBlock("Camera").bind(1);
  phong_yuv->activeUniformBlock("LightColor").bind(2);
  phong_yuv->activeUniformBlock("LightDirection").bind(3);
  phong_yuv->activeUniformBlock("Material").bind(4);
  phong_yuv->activeUniformBlock("Model").bind(5);
  cout << "Phong:" << endl << *phong_yuv << endl;
}

void makeGrid(const GLfloat x_min, const GLfloat y_min, const GLfloat z_min,
              const GLfloat x_max, const GLfloat y_max, const GLfloat z_max,
              const GLfloat u_min, const GLfloat u_max,
              const GLfloat v_min, const GLfloat v_max,
              const int nx, const int ny,
              vector<Vec3f>* obj_pos,
              vector<Vec3f>* yuv,
              vector<Triangle>* tri_index)
{
  const GLfloat x_dim = x_max - x_min;
  const GLfloat y_dim = y_max - y_min;
  const GLfloat dx = x_dim / nx;
  const GLfloat dy = y_dim / ny;
  const GLfloat z = 0.0f; // TMP
  const GLfloat u_dim = u_max - u_min;
  const GLfloat v_dim = v_max - v_min;
  const GLfloat y = 0.5f; // TMP
  const GLfloat du = u_dim / nx;
  const GLfloat dv = v_dim / ny;

  srand(0);

  obj_pos->push_back(Vec3f(x_min, y_min, 0.0f));
  obj_pos->push_back(Vec3f(x_max, y_min, 0.0f));
  obj_pos->push_back(Vec3f(x_max, y_max, 0.0f));
  obj_pos->push_back(Vec3f(x_min, y_max, 0.0f));
  obj_pos->push_back(Vec3f(x_min + 0.5f * x_dim, y_min + 0.5f * y_dim, 0.0f));

  yuv->push_back(Vec3f(0.5f, u_min, v_min));
  yuv->push_back(Vec3f(0.5f, u_max, v_min));
  yuv->push_back(Vec3f(0.5f, u_max, v_max));
  yuv->push_back(Vec3f(0.5f, u_min, v_max));
  yuv->push_back(Vec3f(0.5f, u_min + 0.5f * u_dim, v_min + 0.5f * v_dim));

  auto triangle_sorter =
    [&](Triangle a, Triangle b) -> bool {
      vector<Vec3f>& vtx = *obj_pos;
      return triangleArea(vtx[a.i], vtx[a.j], vtx[a.k]) <
             triangleArea(vtx[b.i], vtx[b.j], vtx[b.k]);
    };

  vector<Triangle> triangles;
  triangles.push_back(Triangle(4, 0, 1));
  triangles.push_back(Triangle(4, 1, 2));
  triangles.push_back(Triangle(4, 2, 3));
  triangles.push_back(Triangle(4, 3, 0));
  sort(triangles.begin(), triangles.end(), triangle_sorter);

  vector<Vec3f>& vtx = *obj_pos;
  vector<Vec3f>& tex = *yuv;
  for (int n = 0; n < 64; ++n) {
    Triangle t = triangles.back();
    triangles.pop_back();
    Vec3f c = triangleCenter(vtx[t.i], vtx[t.j], vtx[t.k]);
    Vec3f ct = triangleCenter(tex[t.i], tex[t.j], tex[t.k]);
    c[2] = 0.5f; // TMP
    vtx.push_back(c);
    tex.push_back(ct);
    GLushort ci = vtx.size() - 1;
    triangles.push_back(Triangle(ci, t.i, t.j));
    triangles.push_back(Triangle(ci, t.j, t.k));
    triangles.push_back(Triangle(ci, t.k, t.i));
    sort(triangles.begin(), triangles.end(), triangle_sorter);
  }

  *tri_index = triangles;

#if 0
  const GLfloat yuv[13 * 3] = {
    0.5f, u_min, v_min,
    0.5f, u_mid, v_min,
    0.5f, u_max, v_min,
    0.5f, 0.25f * (u_min + u_max), 0.25f * (v_min + v_max),
    0.5f, 0.75f * (u_min + u_max), 0.25f * (v_min + v_max),
    0.5f, u_min, v_mid,
    0.5f, u_mid, v_mid,
    0.5f, u_max, v_mid,
    0.5f, 0.25f * (u_min + u_max), 0.75f * (v_min + v_max),
    0.5f, 0.75f * (u_min + u_max), 0.75f * (v_min + v_max),
    0.5f, u_min, v_max,
    0.5f, u_mid, v_max,
    0.5f, u_max, v_max,};
  yuv_vbo.reset(new ArrayBuffer(13 * 3 * sizeof(GLfloat), yuv));

  //const GLfloat obj_pos[5 * 3] = {
  //  y_min, y_min, 0.0f,
  //  y_min, y_max, -8.0f,
  //  y_max, y_max, -5.0f,
  //  y_max, y_min, 0.0f,
  //  -5.0f, -5.0f, -5.0f };

  const GLushort index[4 * 3] = {
    4, 0, 1,
    4, 1, 2,
    4, 2, 3,
    4, 3, 0 };
  index_vbo.reset(new ElementArrayBuffer(4 * 3 * sizeof(GLushort), index));
  cout << "index count: "
       << static_cast<GLsizei>(index_vbo->sizeInBytes() / sizeof(GLushort))
       << endl;
  cout << "triangle count: "
       << static_cast<GLsizei>(index_vbo->sizeInBytes() / sizeof(GLushort)) / 3
       << endl;
#endif



#if 0
  vector<GLfloat> obj_pos;
  vector<GLfloat> yuv;

  // Grid.
  for (int gy = 0; gy <= ny; ++gy) {
    for (int gx = 0; gx <= nx; ++gx) {
      obj_pos.push_back(x_min + gx * dx);
      obj_pos.push_back(y_min + gy * dy);
      obj_pos.push_back(z);
      yuv.push_back(y);
      yuv.push_back(u_min + gx * du);
      yuv.push_back(v_min + gy * dv);
    }
  }

  cout << "grid vertices: " << obj_pos.size() / 3 << endl;

  vector<GLushort> tri_indices;
  int pivot_index = 0;

  // Pivots.
  for (int py = 0; py < ny; ++py) {
    for (int px = 0; px < nx; ++px) {
      obj_pos.push_back(x_min + (px + 0.5f) * dx);
      obj_pos.push_back(y_min + (py + 0.5f) * dy);
      obj_pos.push_back(z);
      yuv.push_back(y);
      yuv.push_back(u_min + (px + 0.5f) * du);
      yuv.push_back(v_min + (py + 0.5f) * dv);

      tri_indices.push_back(pivot_index);
      tri_indices.push_back(ix);
      tri_indices.push_back(ix + 1);

      tri_indices.push_back(pivot_index,);
      tri_indices.push_back(ix);
      tri_indices.push_back();

      tri_indices.push_back(pivot_index,);
      tri_indices.push_back();
      tri_indices.push_back();

      tri_indices.push_back(pivot_index,);
      tri_indices.push_back();
      tri_indices.push_back();

      ++pivot_index;
    }
  }
#endif
}

void initScene()
{
  buildShaderPrograms();

  const GLfloat x_min = -10.0f;
  const GLfloat y_min = -10.0f;
  const GLfloat z_min = -10.0f;
  const GLfloat x_max = 10.0f;
  const GLfloat y_max = 10.0f;
  const GLfloat z_max = 10.0f;
  const GLfloat u_min = -0.436f;
  const GLfloat u_max =  0.436f;
  const GLfloat v_min = -0.615f;
  const GLfloat v_max =  0.615f;

  // --------------------------
  // Initialize uniform blocks.
  // --------------------------

  // Camera.
  array<GLfloat, 2 * 16> camera = {
    1.0f, 0.0f, 0.0f, 0.0f, // view_from_world matrix, column 0.
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 0.0f, 0.0f, // clip_from_view, column 0.
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f };
  makeOrthographicProjectionMatrix(
    x_min, x_max,
    y_min, y_max,
    z_min, z_max,
    &camera[16]);
  camera_ubo.reset(new UniformBuffer(
    camera.size() * sizeof(GLfloat), camera.data()));
  bindUniformBuffer(*phong_yuv, "Camera", *camera_ubo);

  // Light color.
  const array<GLfloat, 1 * 4> light_color = {
    1.0f, 1.0f, 1.0f, 1.0f }; // Field: light_diffuse_color.
  light_color_ubo.reset(new UniformBuffer(
    light_color.size() * sizeof(GLfloat), light_color.data()));
  bindUniformBuffer(*phong_yuv, "LightColor", *light_color_ubo);

  // Light direction.
  const array<GLfloat, 1* 4> light_direction = {
    0.0f, 0.0f, -1.0f, 1.0f }; // Field: light_direction.
  light_direction_ubo.reset(new UniformBuffer(
    light_direction.size() * sizeof(GLfloat), light_direction.data()));
  bindUniformBuffer(*phong_yuv, "LightDirection", *light_direction_ubo);

  // Material.
  const array<GLfloat, 1 * 4> material = {
    1.0f, 1.0f, 1.0f, 1.0f, // Field: front_diffuse.
  };
  material_ubo.reset(new UniformBuffer(
    material.size() * sizeof(GLfloat), material.data()));
  bindUniformBuffer(*phong_yuv, "Material", *material_ubo);

  // Model.
  const array<GLfloat, 2 * 16> model = {
    1.0f, 0.0f, 0.0f, 0.0f, // Field: world_from_obj matrix, column 0.
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 0.0f, 0.0f, // Field: world_from_obj_normal, column 0.
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f };
  model_ubo.reset(new UniformBuffer(
    model.size() * sizeof(GLfloat), model.data()));
  bindUniformBuffer(*phong_yuv, "Model", *model_ubo);

  // ----------------------
  // Initialize attributes.
  // ----------------------

  vector<Vec3f> obj_pos;
  vector<Vec3f> yuv;
  vector<Triangle> tri_index;

  makeGrid(x_min, y_min, z_min,
           x_max, y_max, z_max,
           u_min, u_max,
           v_min, v_max,
           2, 2,
           &obj_pos, &yuv, &tri_index);

  obj_pos_vbo.reset(new ArrayBuffer(
    obj_pos.size() * sizeof(Vec3f), &obj_pos[0]));
  yuv_vbo.reset(new ArrayBuffer(
    yuv.size() * sizeof(Vec3f), &yuv[0]));
  tri_index_ibo.reset(new ElementArrayBuffer(
    tri_index.size() * sizeof(Triangle), &tri_index[0]));

  // Create vertex array to remember bindings.
  phong_yuv_va.reset(new VertexArray);
  phong_yuv_va->bind();

  // Bind obj_pos attribute.
  const Attrib obj_pos_attrib = phong_yuv->activeAttrib("obj_pos");
  const Bindor<ArrayBuffer> obj_pos_vbo_bindor(*obj_pos_vbo);
  const VertexAttribArrayEnabler obj_pos_vaae(obj_pos_attrib.location);
  vertexAttribPointer(
    obj_pos_attrib.location,
    3,        // Number of components.
    VertexAttribType<GLfloat>::VALUE,
    GL_FALSE, // Normalize.
    0,        // Stride.
    0);       // Read from currently bound VBO.

  // Bind yuv attribute.
  const Attrib* yuv_attrib = phong_yuv->queryActiveAttrib("yuv");
  const Bindor<ArrayBuffer> yuv_vbo_bindor(*yuv_vbo);
  const VertexAttribArrayEnabler yuv_vaae(yuv_attrib->location);
  vertexAttribPointer(
    yuv_attrib->location,
    3,        // Number of components.
    VertexAttribType<GLfloat>::VALUE,
    GL_FALSE, // Normalize.
    0,        // Stride.
    0);       // Read from currently bound VBO.

  // Bind triangle indices.
  const Bindor<ElementArrayBuffer> tri_index_bindor(*tri_index_ibo);
  phong_yuv_va->release();

#if 1
  cout << "obj_pos count: "
       << obj_pos_vbo->sizeInBytes() / sizeof(Vec3f)
       << endl
       << "yuv count: "
       << yuv_vbo->sizeInBytes() / sizeof(Vec3f)
       << endl
       << "tri_index count: "
       << 3 * (tri_index_ibo->sizeInBytes() / sizeof(Vec3us))
       << endl
       << "triangle count: "
       << tri_index_ibo->sizeInBytes() / sizeof(Vec3us)
       << endl;
#endif
}

void renderScene()
{
  clear(GL_COLOR_BUFFER_BIT);

  const Bindor<ShaderProgram> phong_yuv_bindor(*phong_yuv);
  const Bindor<VertexArray> phong_yuv_va_bindor(*phong_yuv_va);

  const GLuint min_index = 0;
  const GLuint max_index = (obj_pos_vbo->sizeInBytes() / sizeof(Vec3f)) - 1;
  const GLsizei index_count = 3 * tri_index_ibo->sizeInBytes() / sizeof(Vec3us);
  drawRangeElements(
    GL_TRIANGLES,
    min_index,
    max_index,
    index_count,
    GLTypeEnum<GLushort>::value,
    0); // Read indices from currently bound element array.
}

int main(int argc, char* argv[])
{
  try {

    initGLFW(winWidth, winHeight);
    initGLEW();
    initGL();
    initScene();

    while (!glfwWindowShouldClose(win))
    {
      renderScene();

      // Swap front and back buffers
      // Poll for and process events
      glfwSwapBuffers(win);
      glfwPollEvents();
    }

    // Close window and terminate GLFW.
    glfwDestroyWindow(win);
    glfwTerminate();
  }
  catch (const std::exception& ex) {
    cerr << "Exception: " << ex.what() << "\n";
    // Close window and terminate GLFW.
    glfwDestroyWindow(win);
    glfwTerminate();
    abort();
  }
  catch (const ndj::Exception& ex) {
    cerr << "Exception: " << ex.what() << "\n";
    // Close window and terminate GLFW.
    glfwDestroyWindow(win);
    glfwTerminate();
    abort();
  }
  catch (...) {
    cerr << "Unknown exception\n";
    // Close window and terminate GLFW.
    glfwDestroyWindow(win);
    glfwTerminate();
    abort();
  }

  return EXIT_SUCCESS;
}
