#include <array>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <random>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <nDjinn.hpp>
#include <thx.hpp>
#include <thinks/poissonDiskSampling.hpp>

#include "triangle/triangle.h"

using namespace std;
using namespace ndj;
using namespace thx;

GLFWwindow* win = nullptr;
int win_width = 480;
int win_height = 480;

GLsizei const kMaxFboWidth = 512;//8192;
GLsizei const kMaxFboHeight = 512;//8192;
GLsizei fbo_width = 0;
GLsizei fbo_height = 0;

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
unique_ptr<Framebuffer> fbo;
unique_ptr<Texture2D> rgb_tex;
//unique_ptr<Renderbuffer> rbo;
unique_ptr<ShaderProgram> screen_tex;
unique_ptr<UniformBuffer> screen_tex_camera_ubo;
unique_ptr<UniformBuffer> screen_tex_model_ubo;
unique_ptr<VertexArray> screen_tex_va;
unique_ptr<ArrayBuffer> screen_tex_obj_pos_vbo;
unique_ptr<ArrayBuffer> screen_tex_uv_vbo;
unique_ptr<ElementArrayBuffer> screen_tex_tri_ibo;

typedef vec<2, GLfloat> Vec2f;
typedef vec<3, GLfloat> Vec3f;
typedef vec<3, GLushort> Vec3us;

GLfloat triangleArea(const Vec3f& v0, const Vec3f& v1, const Vec3f& v2) {
  return 0.5f * mag(cross(v2 - v0, v2 - v1));
}

Vec3f triangleCenter(const Vec3f& v0, const Vec3f& v1, const Vec3f& v2) {
  return (1.0f / 3) * (v0 + v1 + v2);
}

struct Triangle
{
  Triangle() : i(0), j(0), k(0) {}
  Triangle(const GLushort i, const GLushort j, const GLushort k)
    : i(i), j(j), k(k) {}
  GLushort i;
  GLushort j;
  GLushort k;
};

template <typename T>
struct Pixel
{
  Pixel() : r(0), g(0), b(0) {}
  Pixel(const T _r, const T _g, const T _b) : r(_r), g(_g), b(_b) {}
  T r;
  T g;
  T b;
};

typedef Pixel<float> Pixelf;
typedef Pixel<uint8_t> Pixel8ui;


void writeObj(const string& filename, const vector<Vec3f>& vtx,
              const vector<Triangle>& tris)
{
  ofstream ofs(filename);
  for (size_t v = 0; v < vtx.size(); ++v) {
    ofs << "v " << vtx[v][0] << " " << vtx[v][1] << " " << vtx[v][2] << endl;
  }
  ofs << endl;
  for (size_t t = 0; t < tris.size(); ++t) {
    ofs << "f " << tris[t].i + 1 << " " << tris[t].j + 1 << " " << tris[t].k + 1 << endl;
  }
  ofs.close();
}

void writePpm(const string& filename, size_t width, size_t height,
              const vector<Pixel8ui>& pixels)
{
  ofstream ofs(filename, ios_base::out | ios_base::binary | ios_base::trunc);
  ofs << "P6" << endl;
  ofs << width << " " << height << endl;
  ofs << 255 << endl;
  ofs.write(reinterpret_cast<const char*>(pixels.data()),
            pixels.size() * sizeof(Pixel8ui));
  ofs.close();
}

void writePpm(const string& filename, size_t width, size_t height,
              const vector<Pixelf>& pixels)
{
  vector<Pixel8ui> pixels8ui(pixels.size());
  transform(begin(pixels), end(pixels), begin(pixels8ui),
    [](const Pixelf p) {
      return Pixel8ui(static_cast<uint8_t>(p.r * 255.f),
                      static_cast<uint8_t>(p.g * 255.f),
                      static_cast<uint8_t>(p.b * 255.f));
    });
  writePpm(filename, width, height, pixels8ui);
}


//! DOCS
void framebufferSizeCallback(GLFWwindow* win,
                             const int w, const int h)
{
  //cout << "framebufferSizeCallback" << endl;

  win_width = w;
  win_height = h;
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
  if (!glfwInit())
  {
    throw runtime_error("GLFW init error");
  }

  glfwWindowHint(GLFW_SAMPLES, 4);

  win = glfwCreateWindow(width, height, "yuv-valence", nullptr, nullptr);
  if (win == nullptr)
  {
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
    cerr << "Warning: " << ex.what() << endl;
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

  glfwGetFramebufferSize(win, &win_width, &win_height);
  viewport(0, 0, win_width, win_height);

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

  GLint const maxVertexAttribs = getInteger(GL_MAX_VERTEX_ATTRIBS);
  GLint const max_renderbuffer_size = getInteger(GL_MAX_RENDERBUFFER_SIZE);
  GLint const max_draw_buffers = getInteger(GL_MAX_DRAW_BUFFERS);
  GLint const max_texture_size = getInteger(GL_MAX_TEXTURE_SIZE);
  array<GLint, 2> const max_viewport_dims =
    getIntegers<2>(GL_MAX_VIEWPORT_DIMS);

  fbo_width = std::min(max_texture_size, max_viewport_dims[0]);
  fbo_height = std::min(max_texture_size, max_viewport_dims[1]);
  fbo_width = std::min(fbo_width, fbo_height);
  fbo_height = std::min(fbo_width, fbo_height);
  fbo_width = std::min(fbo_width, kMaxFboWidth);
  fbo_height = std::min(fbo_height, kMaxFboHeight);

  cout << "GL_MAX_VERTEX_ATTRIBS: " << maxVertexAttribs << "\n";
  cout << "GL_MAX_RENDERBUFFER_SIZE: " << max_renderbuffer_size << "\n";
  cout << "GL_MAX_DRAW_BUFFERS: " << max_draw_buffers << "\n";
  cout << "GL_MAX_TEXTURE_SIZE: " << max_texture_size << endl;
  cout << "GL_MAX_VIEWPORT_DIMS: "
       << max_viewport_dims[0] << ", " << max_viewport_dims[1] << endl;
  cout << "FBO width: " << fbo_width << endl;
  cout << "FBO height: " << fbo_height << endl;

  rgb_tex.reset(new Texture2D(GL_TEXTURE_2D, fbo_width, fbo_height, 0, GL_RGB,
                              0, GL_RGB, GL_UNSIGNED_BYTE, nullptr));
  setTextureParameter(*rgb_tex, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  setTextureParameter(*rgb_tex, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  cout << endl << *rgb_tex << endl;

  fbo.reset(new Framebuffer);
  fbo->attachTexture2D(GL_COLOR_ATTACHMENT0, rgb_tex->handle());
  cout << endl << *fbo << endl;
}

void triangulate(const vector<Vec2f>& pos, vector<Triangle>* tri_index)
{
  using namespace std;
  triangulateio triangulate_in;
  triangulate_in.pointlist = reinterpret_cast<REAL*>(
    malloc(pos.size() * 2 * sizeof(REAL)));
  triangulate_in.pointattributelist = nullptr;
  triangulate_in.pointmarkerlist = nullptr;
  triangulate_in.numberofpoints = static_cast<int>(pos.size());
  triangulate_in.numberofpointattributes = 0;
  triangulate_in.trianglelist = nullptr;
  triangulate_in.triangleattributelist = nullptr;
  triangulate_in.trianglearealist = nullptr;
  triangulate_in.numberoftriangles = 0;
  triangulate_in.numberofcorners = 0;
  triangulate_in.numberoftriangleattributes = 0;
  triangulate_in.segmentlist = nullptr;
  triangulate_in.segmentmarkerlist = nullptr;
  triangulate_in.numberofsegments = 0;
  triangulate_in.holelist = nullptr;
  triangulate_in.numberofholes = 0;
  triangulate_in.regionlist = nullptr;
  triangulate_in.numberofregions = 0;

  for (size_t i = 0; i < pos.size(); ++i) {
    triangulate_in.pointlist[i * 2 + 0] = pos[i][0];
    triangulate_in.pointlist[i * 2 + 1] = pos[i][1];
  }

  triangulateio triangulate_out;
  triangulate_out.pointlist = nullptr; // Not needed if -N switch used.
  triangulate_out.pointattributelist = nullptr; // Not needed if -N switch used or number of point attributes is zero.
  triangulate_out.pointmarkerlist = nullptr; // Not needed if -N or -B switch used.
  triangulate_out.trianglelist = nullptr; // Not needed if -E switch used.
  triangulate_out.triangleattributelist = nullptr; // Not needed if -E switch used or number of triangle attributes is zero.
  triangulate_out.neighborlist = nullptr; // Needed only if -n switch used.
  triangulate_out.segmentlist = nullptr; // Needed only if segments are output (-p or -c) and -P not used:
  triangulate_out.segmentmarkerlist = nullptr;   // Needed only if segments are output (-p or -c) and -P and -B not used:
  triangulate_out.edgelist = nullptr; // Needed only if -e switch used.
  triangulate_out.edgemarkerlist = nullptr; // Needed if -e used and -B not used.
  triangulate_out.normlist = nullptr;

  vector<char> triangulate_flags;
  triangulate_flags.push_back('P'); // Suppresses the output .poly file.
  triangulate_flags.push_back('N'); // Suppresses the output .node file.
  //triangulate_flags.push_back('E'); // Suppresses the output .ele file.
  triangulate_flags.push_back('c');
  triangulate_flags.push_back('Q'); // Quiet.
  triangulate_flags.push_back('z'); // Zero-based indexing.
  triangulate_flags.push_back('\0'); // Null-termination.

  triangulate(
    triangulate_flags.data(),
    &triangulate_in,
    &triangulate_out,
    nullptr);

  assert(tri_index != nullptr);
  tri_index->clear();
  tri_index->resize(triangulate_out.numberoftriangles);
  assert(triangulate_out.numberofcorners == 3);
  for (int t = 0; t < triangulate_out.numberoftriangles; ++t) {
    (*tri_index)[t].i = triangulate_out.trianglelist[3 * t + 0];
    (*tri_index)[t].j = triangulate_out.trianglelist[3 * t + 1];
    (*tri_index)[t].k = triangulate_out.trianglelist[3 * t + 2];
  }

  // Free all allocated arrays, including those allocated by Triangle.
  free(triangulate_in.pointlist);
  free(triangulate_out.pointlist);
  free(triangulate_in.pointattributelist);
  free(triangulate_out.pointattributelist);
  free(triangulate_in.pointmarkerlist);
  free(triangulate_out.pointmarkerlist);
  free(triangulate_in.trianglelist);
  free(triangulate_out.trianglelist);
  free(triangulate_in.triangleattributelist);
  free(triangulate_out.triangleattributelist);
  free(triangulate_in.trianglearealist); // In only.
  free(triangulate_out.neighborlist); // Out only.
  free(triangulate_in.segmentlist);
  free(triangulate_out.segmentlist);
  free(triangulate_in.segmentmarkerlist);
  free(triangulate_out.segmentmarkerlist);
  free(triangulate_in.holelist); // In only.
  free(triangulate_in.regionlist); // In only.
  free(triangulate_out.edgelist); // Out only.
  free(triangulate_out.edgemarkerlist); // Out only.
  free(triangulate_out.normlist); // Out only.
}

void makeMesh(const GLfloat x_min, const GLfloat y_min, const GLfloat z_min,
              const GLfloat x_max, const GLfloat y_max, const GLfloat z_max,
              const GLfloat u_min, const GLfloat u_max,
              const GLfloat v_min, const GLfloat v_max,
              const GLfloat radius,
              const uint32_t seed,
              vector<Vec3f>* obj_pos,
              vector<Vec3f>* yuv,
              vector<Triangle>* tri_index)
{
  assert(obj_pos != nullptr);
  assert(yuv != nullptr);
  const array<GLfloat, 2> sampling_min = { x_min - 2.f * radius,
                                           y_min - 2.f * radius };
  const array<GLfloat, 2> sampling_max = { x_max + 2.f * radius,
                                           y_max + 2.f * radius};
  const vector<array<GLfloat, 2>> samples =
    thinks::poissonDiskSampling(radius, sampling_min, sampling_max, 30, seed);
  vector<Vec2f> obj_pos_xy(samples.size());
  for (size_t i = 0; i < samples.size(); ++i) {
    obj_pos_xy[i][0] = samples[i][0];
    obj_pos_xy[i][1] = samples[i][1];
  }

  triangulate(obj_pos_xy, tri_index);

  // Compute triangle vertices in 3D, adding a random offset in Z.
  random_device rd;
  mt19937 gen(rd());
  uniform_real_distribution<GLfloat> dis(z_min, z_max);
  obj_pos->clear();
  obj_pos->resize(obj_pos_xy.size());
  yuv->clear();
  yuv->resize(samples.size());
  for (size_t i = 0; i < obj_pos_xy.size(); ++i) {
    (*obj_pos)[i][0] = obj_pos_xy[i][0];
    (*obj_pos)[i][1] = obj_pos_xy[i][1];
    (*obj_pos)[i][2] = dis(gen);
    const GLfloat tu = ((*obj_pos)[i][0] - x_min) / (x_max - x_min);
    const GLfloat tv = ((*obj_pos)[i][1] - y_min) / (y_max - y_min);
    (*yuv)[i][0] = 0.5f;
    (*yuv)[i][1] = u_min + (u_max - u_min) * tu;
    (*yuv)[i][2] = v_min + (v_max - v_min) * tv;
  }
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
  cout << "phong_yuv:" << endl << *phong_yuv << endl;

  screen_tex.reset(new ShaderProgram(
    VertexShader(readShaderFile("shaders/screen_tex.vs")),
    FragmentShader(readShaderFile("shaders/screen_tex.fs"))));
  screen_tex->activeUniformBlock("Camera").bind(6);
  screen_tex->activeUniformBlock("Model").bind(7);
  cout << "screen_tex:" << endl << *screen_tex << endl;
}

void initScene()
{
  const GLfloat x_min = -10.f;
  const GLfloat y_min = -10.f;
  const GLfloat z_min = -1.f;
  const GLfloat x_max = 10.f;
  const GLfloat y_max = 10.f;
  const GLfloat z_max = 1.f;
  const GLfloat u_min = -0.436f;
  const GLfloat u_max =  0.436f;
  const GLfloat v_min = -0.615f;
  const GLfloat v_max =  0.615f;
  const GLfloat radius = 1.5f;
  const uint32_t seed = 1954;

  // --------------------------
  // Initialize uniform blocks.
  // --------------------------

  // Camera.
  array<GLfloat, 2 * 16> camera = {
    1.f, 0.f, 0.f, 0.f, // view_from_world matrix, column 0.
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    0.f, 0.f, 0.f, 1.f,
    1.f, 0.f, 0.f, 0.f, // clip_from_view, column 0.
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    0.f, 0.f, 0.f, 1.f };
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
    1.f, 1.f, 1.f, 1.f }; // Field: light_diffuse_color.
  light_color_ubo.reset(new UniformBuffer(
    light_color.size() * sizeof(GLfloat), light_color.data()));
  bindUniformBuffer(*phong_yuv, "LightColor", *light_color_ubo);

  // Light direction.
  const array<GLfloat, 1 * 4> light_direction = {
    0.f, 0.f, -1.f, 1.f }; // Field: light_direction.
  light_direction_ubo.reset(new UniformBuffer(
    light_direction.size() * sizeof(GLfloat), light_direction.data()));
  bindUniformBuffer(*phong_yuv, "LightDirection", *light_direction_ubo);

  // Material.
  const array<GLfloat, 1 * 4> material = {
    1.f, 1.f, 1.f, 1.f, // Field: front_diffuse.
  };
  material_ubo.reset(new UniformBuffer(
    material.size() * sizeof(GLfloat), material.data()));
  bindUniformBuffer(*phong_yuv, "Material", *material_ubo);

  // Model.
  const array<GLfloat, 2 * 16> model = {
    1.f, 0.f, 0.f, 0.f, // Field: world_from_obj matrix, column 0.
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    0.f, 0.f, 0.f, 1.f,
    1.f, 0.f, 0.f, 0.f, // Field: world_from_obj_normal, column 0.
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    0.f, 0.f, 0.f, 1.f };
  model_ubo.reset(new UniformBuffer(
    model.size() * sizeof(GLfloat), model.data()));
  bindUniformBuffer(*phong_yuv, "Model", *model_ubo);

  // ----------------------
  // Initialize attributes.
  // ----------------------

  vector<Vec3f> obj_pos;
  vector<Vec3f> yuv;
  vector<Triangle> tri_index;
  makeMesh(x_min, y_min, z_min,
           x_max, y_max, z_max,
           u_min, u_max,
           v_min, v_max,
           radius, seed,
           &obj_pos, &yuv, &tri_index);
  //writeObj("mesh.obj", obj_pos, tri_index); // TMP!!

  obj_pos_vbo.reset(new ArrayBuffer(
    obj_pos.size() * sizeof(Vec3f), &obj_pos[0]));
  yuv_vbo.reset(new ArrayBuffer(
    yuv.size() * sizeof(Vec3f), &yuv[0]));
  tri_index_ibo.reset(new ElementArrayBuffer(
    tri_index.size() * sizeof(Triangle), &tri_index[0]));

  // Create vertex array to "remember" bindings.
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

void initScreen()
{
  // --------------------------
  // Initialize uniform blocks.
  // --------------------------

  // Camera.
  array<GLfloat, 2 * 16> camera = {
    1.f, 0.f, 0.f, 0.f, // view_from_world matrix, column 0.
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    0.f, 0.f, 0.f, 1.f,
    1.f, 0.f, 0.f, 0.f, // clip_from_view, column 0.
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    0.f, 0.f, 0.f, 1.f };
  makeOrthographicProjectionMatrix(
    -1.f, 1.f,
    -1.f, 1.f,
    -1.f, 1.f,
    &camera[16]);
  screen_tex_camera_ubo.reset(new UniformBuffer(
    camera.size() * sizeof(GLfloat), camera.data()));
  bindUniformBuffer(*screen_tex, "Camera", *screen_tex_camera_ubo);

  // Model.
  const array<GLfloat, 1 * 16> model = {
    1.f, 0.f, 0.f, 0.f, // Field: world_from_obj matrix, column 0.
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    0.f, 0.f, 0.f, 1.f };
  screen_tex_model_ubo.reset(new UniformBuffer(
    model.size() * sizeof(GLfloat), model.data()));
  bindUniformBuffer(*screen_tex, "Model", *screen_tex_model_ubo);

  // ----------------------
  // Initialize attributes.
  // ----------------------

  array<Vec3f, 4> obj_pos;
  array<Vec2f, 4> uv;
  array<Triangle, 2> tri_index;
  obj_pos[0] = Vec3f(-1.f, -1.f, 0.f);
  obj_pos[1] = Vec3f(+1.f, -1.f, 0.f);
  obj_pos[2] = Vec3f(+1.f, +1.f, 0.f);
  obj_pos[3] = Vec3f(-1.f, +1.f, 0.f);
  uv[0] = Vec2f(0.f, 0.f);
  uv[1] = Vec2f(1.f, 0.f);
  uv[2] = Vec2f(1.f, 1.f);
  uv[3] = Vec2f(0.f, 1.f);
  tri_index[0] = Triangle(0, 1, 2);
  tri_index[1] = Triangle(2, 3, 0);

  //writeObj("screen_tex.obj", obj_pos, tri_index); // TMP!!

  screen_tex_obj_pos_vbo.reset(new ArrayBuffer(
    obj_pos.size() * sizeof(Vec3f), obj_pos.data()));
  screen_tex_uv_vbo.reset(new ArrayBuffer(
    uv.size() * sizeof(Vec2f), uv.data()));
  screen_tex_tri_ibo.reset(new ElementArrayBuffer(
    tri_index.size() * sizeof(Triangle), tri_index.data()));

  // Create vertex array to "remember" bindings.
  screen_tex_va.reset(new VertexArray);
  screen_tex_va->bind();

  // Bind obj_pos attribute.
  const Attrib obj_pos_attrib = screen_tex->activeAttrib("obj_pos");
  const Bindor<ArrayBuffer> obj_pos_vbo_bindor(*screen_tex_obj_pos_vbo);
  const VertexAttribArrayEnabler obj_pos_vaae(obj_pos_attrib.location);
  vertexAttribPointer(
    obj_pos_attrib.location,
    3,        // Number of components.
    VertexAttribType<GLfloat>::VALUE,
    GL_FALSE, // Normalize.
    0,        // Stride.
    nullptr); // Read from currently bound VBO.

  // Bind uv attribute.
  const Attrib* uv_attrib = screen_tex->queryActiveAttrib("uv");
  const Bindor<ArrayBuffer> uv_vbo_bindor(*screen_tex_uv_vbo);
  const VertexAttribArrayEnabler uv_vaae(uv_attrib->location);
  vertexAttribPointer(
    uv_attrib->location,
    2,        // Number of components.
    VertexAttribType<GLfloat>::VALUE,
    GL_FALSE, // Normalize.
    0,        // Stride.
    nullptr); // Read from currently bound VBO.

  // Bind triangle indices.
  const Bindor<ElementArrayBuffer> tri_index_bindor(*screen_tex_tri_ibo);
  screen_tex_va->release();
}

void drawScene()
{
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
    nullptr); // Read indices from currently bound element array.
}

void drawScreen()
{
  activeTexture(GL_TEXTURE0);
  const Bindor<ShaderProgram> screen_tex_bindor(*screen_tex);
  const Bindor<VertexArray> screen_tex_va_bindor(*screen_tex_va);
  const TextureBindor<Texture2D> tex_bindor(GL_TEXTURE_2D, *rgb_tex);

  const GLuint min_index = 0;
  const GLuint max_index =
    (screen_tex_obj_pos_vbo->sizeInBytes() / sizeof(Vec3f)) - 1;
  const GLsizei index_count =
    3 * screen_tex_tri_ibo->sizeInBytes() / sizeof(Vec3us);
  drawRangeElements(
    GL_TRIANGLES,
    min_index,
    max_index,
    index_count,
    GLTypeEnum<GLushort>::value,
    nullptr); // Read indices from currently bound element array.
}

void renderTexture()
{
  fbo->bind(GL_DRAW_FRAMEBUFFER);
  viewport(0, 0, fbo_width, fbo_height);
  array<GLenum, 1> draw_bufs = { GL_COLOR_ATTACHMENT0 };
  drawBuffers(draw_bufs);
  array<GLfloat, 4> clear_color = { .5f, .5f, .5f, 1.f };
  clearBufferfv(GL_COLOR, 0, clear_color.data());
  drawScene();
  fbo->release(GL_DRAW_FRAMEBUFFER);
}

void renderScreen()
{
  viewport(0, 0, win_width, win_height);
  array<GLenum, 1> draw_bufs = { GL_BACK };
  drawBuffers(draw_bufs);
  drawScreen();
}

void writeTexture(string const& filename)
{
  vector<Pixel8ui> img(fbo_width * fbo_height);
  fbo->bind(GL_READ_FRAMEBUFFER);
  readBuffer(GL_COLOR_ATTACHMENT0);
  //namedFramebufferReadBuffer(fbo->handle(), GL_COLOR_ATTACHMENT0);
  readPixels(0, 0, fbo_width, fbo_height, GL_RGB, GL_UNSIGNED_BYTE,
             reinterpret_cast<GLvoid*>(img.data()));
  writePpm(filename, fbo_width, fbo_height, img);
  fbo->release(GL_READ_FRAMEBUFFER);
}

int main(int argc, char* argv[])
{
  try {
    initGLFW(win_width, win_height);
    initGLEW();
    initGL();
    buildShaderPrograms();
    initScene();
    initScreen();

    while (!glfwWindowShouldClose(win))
    {
      renderTexture();
      renderScreen();

      // Swap front and back buffers, poll for and process events.
      glfwSwapBuffers(win);
      glfwPollEvents();
    }

    writeTexture("tex.ppm");

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


#if 0
  {
    vector<Pixel8ui> img(32 * 32);
    for (size_t i = 0; i < img.size(); ++i)
    {
      img[i] = Pixel8ui(255, 0, 0);
    }
    writePpm("test.ppm", 32, 32, img);
    cout << "wrote test.ppm" << endl;
  }
#endif

#if 0
    glGetTexImage(GL_TEXTURE_2D,
      0,
      GL_RGB,
      GL_UNSIGNED_BYTE,
      (GLvoid*)img.data());
    checkError("glGetTexImage");
    writePpm("framebuffer2.ppm", 4092, 4092, img);
#endif
