#include <exception>
#include <iostream>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <nDjinn.hpp>

using namespace std;
using namespace ndj;

GLFWwindow* win = nullptr;
int winWidth = 480;
int winHeight = 480;

unique_ptr<ShaderProgram> phong;
unique_ptr<VertexArray> phong_va;
unique_ptr<UniformBuffer> camera_ubo;
unique_ptr<UniformBuffer> light_color_ubo;
unique_ptr<UniformBuffer> light_position_ubo;
unique_ptr<UniformBuffer> material_ubo;
unique_ptr<UniformBuffer> model_ubo;
unique_ptr<ArrayBuffer> position_vbo;
unique_ptr<ArrayBuffer> tex2_vbo;
unique_ptr<ElementArrayBuffer> index_vbo;

//! DOCS
void framebufferSizeCallback(GLFWwindow* win,
                             const int w, const int h)
{
  cout << "framebufferSizeCallback" << endl;

  winWidth = w;
  winHeight = h;

#if 0
  using std::cout;

  winSize[0] = w;
  winSize[1] = h;
  cout << "Resize: " << winSize << "\n";

  // Update scene viewport. Covers entire window.
  sceneViewport.x = 0;
  sceneViewport.y = 0;
  sceneViewport.width = static_cast<GLsizei>(winSize[0]);
  sceneViewport.height = static_cast<GLsizei>(winSize[1]);
  cout << "Scene viewport: "
    << vec4i(sceneViewport.x,
             sceneViewport.y,
             sceneViewport.width,
             sceneViewport.height)
    << "\n";

  // Update contour plane viewport. Always in lower left corner.
  GLint const contourPlaneViewportDim =
    std::max(
      static_cast<GLint>((1./3)*sceneViewport.width),
      static_cast<GLint>((1./3)*sceneViewport.height));
  contourPlaneViewport.x =
    sceneViewport.width - contourPlaneViewportDim;
  contourPlaneViewport.y = 0;
  contourPlaneViewport.width = contourPlaneViewportDim;
  contourPlaneViewport.height = contourPlaneViewportDim;
  cout << "Contour plane viewport: "
    << vec4i(contourPlaneViewport.x,
             contourPlaneViewport.y,
             contourPlaneViewport.width,
             contourPlaneViewport.height)
    << "\n";

  // Update scene camera. Take new scene viewport aspect ratio into account.
  Frustum newFrustum(sceneCamera.frustum());
  GLfloat const widthToHeightRatio =
    static_cast<GLfloat>(sceneViewport.width)/sceneViewport.height;
  newFrustum.left = -.5f*widthToHeightRatio;
  newFrustum.right = .5f*widthToHeightRatio;
  sceneCamera.setProjection(newFrustum);

  sliceTexture.reset(
    new ndj::Texture2D(
      GL_TEXTURE_2D,
      2*contourPlaneViewportDim,
      2*contourPlaneViewportDim));
  cout << "Slice texture size: " <<
    vec2i(sliceTexture->width(), sliceTexture->height()) << "\n";
  sliceFbo.reset(new ndj::Framebuffer(GL_FRAMEBUFFER));
  sliceFbo->attachTexture2D(GL_COLOR_ATTACHMENT0, sliceTexture->handle());
#endif
}

//! DOCS
void keyCallback(GLFWwindow* window,
                 int key, int scancode, int action, int mods)
{
  cout << "keyCallback" << endl;


#if 0
  using std::cout;

  switch (action) {
  case GLFW_PRESS:
    switch (k) {
    case 'F':
      //cam.frame(sceneMin, sceneMax);
      break;
    case 'S':
      switch (sliceMode) {
      case GPU:
        sliceMode = CPU;
        cout << "Slice mode: CPU\n";
        break;
      case CPU:
        sliceMode = GPU;
        cout << "Slice mode: GPU\n";
        break;
      }
      break;
    case GLFW_KEY_ESC:
      break;
    case GLFW_KEY_UP:
      break;
    case GLFW_KEY_DOWN:
      break;
    case GLFW_KEY_LEFT:
      break;
    case GLFW_KEY_RIGHT:
      break;
    default:
      break;
    }
    break;
  case GLFW_RELEASE:
    break;
  default:
    break;
  }
#endif
}

//! DOCS
void cursorPosCallback(GLFWwindow* win,
                       const double mx, const double my)
{
  using namespace std;

  //cout << "cursorPosCallback" << endl;

#if 0
  int const dx = mx - dragLastX;
  int const dy = my - dragLastY;
  if (lmbPressed) {
    //GLint vp[4];
    //ndj::Viewport::getViewport(vp);
    vec3f const dr =
      vec3f(static_cast<GLfloat>(dy), static_cast<GLfloat>(dx), 0.f);
    meshModel.setRotation(meshModel.rotation() - dr);
    meshModel.setNormalMatrix(sceneCamera.uniformData().viewMatrix);
  }
  else if (mmbPressed) {
    // TODO: implement camera panning!
  }
  else if (rmbPressed) {
    GLfloat const minMeshDimension =
      thx::min(meshGeometry.max[0] - meshGeometry.min[0],
               meshGeometry.max[1] - meshGeometry.min[1],
               meshGeometry.max[2] - meshGeometry.min[2]);
    vec3f const dp = vec3f(0.f, 0.f, (2.f*minMeshDimension*dy)/winSize[0]);
    sceneCamera.setView(sceneCamera.position() - dp, vec3f(0.f));
  }

  dragLastX = mx;
  dragLastY = my;
#endif
}

//! DOCS
void mouseButtonCallback(GLFWwindow* window,
                         int button, int action, int mods)
{
  using namespace std;

  cout << "mouseButtonCallback" << endl;

#if 0
  glfwGetMousePos(&dragStartX, &dragStartY);
  dragLastX = dragStartX;
  dragLastY = dragStartY;

  switch (button) {
  case GLFW_MOUSE_BUTTON_LEFT:
    switch (state) {
    case GLFW_PRESS:
      lmbPressed = true;
      break;
    case GLFW_RELEASE:
      lmbPressed = false;
      break;
    default:
      break;
    }
    break;
  case GLFW_MOUSE_BUTTON_MIDDLE:
    switch (state) {
    case GLFW_PRESS:
      mmbPressed = true;
      break;
    case GLFW_RELEASE:
      mmbPressed = false;
      break;
    default:
      break;
    }
    break;
  case GLFW_MOUSE_BUTTON_RIGHT:
    switch (state) {
    case GLFW_PRESS:
      rmbPressed = true;
      break;
    case GLFW_RELEASE:
      rmbPressed = false;
      break;
    default:
      break;
    }
    break;
  default:
    break;
  }
#endif
}

//! DOCS
void scrollCallback(GLFWwindow* win, double xoffset, double yoffset)
{
  cout << "scrollCallback" << endl;
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

  win = glfwCreateWindow(width, height, "fstudio", nullptr, nullptr);
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
  using namespace std;

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
  clearDepth(1.);
  depthRange(0., 1.);
  enable(GL_MULTISAMPLE);
  disable(GL_CULL_FACE);
  //disable(GL_NORMALIZE);

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
  phong.reset(new ShaderProgram(
    VertexShader(readShaderFile("shaders/phong.vs")),
    GeometryShader(readShaderFile("shaders/phong.gs")),
    FragmentShader(readShaderFile("shaders/phong.fs"))));
  phong->activeUniformBlock("Camera").bind(1);
  phong->activeUniformBlock("LightColor").bind(2);
  phong->activeUniformBlock("LightPosition").bind(3);
  phong->activeUniformBlock("Material").bind(4);
  phong->activeUniformBlock("Model").bind(5);
  cout << "Phong:" << endl << *phong << endl;
}

#if 0
pair<vector<GLfloat>, vector<GLushort>> makeGrid(const GLfloat x_min,
                                                 const GLfloat x_max,
                                                 const GLfloat y_min,
                                                 const GLfloat y_max,
                                                 const GLfloat z_min,
                                                 const GLfloat z_max,
                                                 const GLfloat u_min,
                                                 const GLfloat u_max,
                                                 const GLfloat v_min,
                                                 const GLfloat v_max,
                                                 const int n_x,
                                                 const int n_y)
{

}
#endif

void initScene()
{
  const GLfloat cam_right[3] = { 1.0f, 0.0f, 0.0f };
  const GLfloat cam_up[3] = { 0.0f, 1.0f, 0.0f };
  const GLfloat cam_back[3] = { 0.0f, 0.0f, 1.0f };
  const GLfloat cam_pos[3] = { 0.0f, 0.0f, 1.0f };
  GLfloat camera[2 * 16] = { 0.0f };
  makeViewMatrix(cam_right, cam_up, cam_back, cam_pos, &camera[0]);
  makeOrthographicProjectionMatrix(
    -100.0f, 100.0f,
    -100.0f, 100.0f,
    1.0f, 100.0f,
    &camera[16]);
  camera_ubo.reset(new UniformBuffer(2 * 16 * sizeof(GLfloat), &camera));
  bindUniformBuffer(*phong, "Camera", *camera_ubo);

  const GLfloat light_color[3 * 4] = {
    1.0f, 1.0f, 1.0f, 1.0f,   // Ambient.
    1.0f, 1.0f, 1.0f, 1.0f,   // Diffuse.
    1.0f, 1.0f, 1.0f, 1.0f }; // Specular.
  light_color_ubo.reset(new UniformBuffer(3 * 4 * sizeof(GLfloat), light_color));
  bindUniformBuffer(*phong, "LightColor", *light_color_ubo);

  const GLfloat light_position[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
  light_position_ubo.reset(new UniformBuffer(4 * sizeof(GLfloat), light_position));
  bindUniformBuffer(*phong, "LightPosition", *light_position_ubo);

  const GLfloat material[24] = {
    0.2f, 0.2f, 0.2f, 1.0f, // Front ambient.
    0.5f, 0.5f, 0.5f, 1.0f, // Front diffuse.
    0.1f, 0.1f, 0.1f, 1.0f, // Front specular.
    0.2f, 0.2f, 0.2f, 1.0f, // Back ambient.
    0.5f, 0.5f, 0.5f, 1.0f, // Back diffuse.
    0.1f, 0.1f, 0.1f, 1.0f  // Back specular.
  };
//  const GLfloat material[6 * 4] = {
//    1.0f, 1.0f, 1.0f, 1.0f,   // Front ambient.
//    0.0f, 0.0f, 0.0f, 1.0f,   // Front diffuse.
//    0.0f, 0.0f, 0.0f, 1.0f,   // Front specular.
//    1.0f, 1.0f, 1.0f, 1.0f,   // Back ambient.
//    0.0f, 0.0f, 0.0f, 1.0f,   // Back diffuse.
//    0.0f, 0.0f, 0.0f, 1.0f }; // Back specular.
  material_ubo.reset(new UniformBuffer(6 * 4 * sizeof(GLfloat), material));
  bindUniformBuffer(*phong, "Material", *material_ubo);

  const GLfloat model[2 * 16] = {
    1.0f, 0.0f, 0.0f, 0.0f, // Model matrix, column 0.
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 0.0f, 0.0f, // Normal matrix, column 0.
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f };
  model_ubo.reset(new UniformBuffer(2 * 16 * sizeof(GLfloat), model));
  bindUniformBuffer(*phong, "Model", *model_ubo);

  const GLfloat x_min = -100.0f;
  const GLfloat x_max =  100.0f;
  const GLfloat y_min = -100.0f;
  const GLfloat y_max =  100.0f;
//  const GLfloat position[4 * 3] = {
//    x_min, y_min, 0.0f,
//    x_max, y_min, 0.0f,
//    x_min, y_max, 0.0f,
//    x_max, y_max, 0.0f };
//  position_vbo.reset(new ArrayBuffer(4 * 3 * sizeof(GLfloat), position));
  const GLfloat position[5 * 3] = {
    x_min, y_min, 0.0f,
    x_max, y_min, 0.0f,
    1.0f,  1.0f, -25.0f,
    x_min, y_max, 0.0f,
    x_max, y_max, 0.0f };
  position_vbo.reset(new ArrayBuffer(5 * 3 * sizeof(GLfloat), position));

  const GLfloat u_min = -0.436f;
  const GLfloat u_max =  0.436f;
  const GLfloat v_min = -0.615f;
  const GLfloat v_max =  0.615f;
//  const GLfloat tex2[4 * 2] = {
//    u_min, v_min,
//    u_max, v_min,
//    u_min, v_max,
//    u_max, v_max };
//  tex2_vbo.reset(new ArrayBuffer(4 * 2 * sizeof(GLfloat), tex2));
  const GLfloat tex2[5 * 2] = {
    u_min, v_min,
    u_max, v_min,
    0.0f,   0.0f,
    u_min, v_max,
    u_max, v_max };
  tex2_vbo.reset(new ArrayBuffer(5 * 2 * sizeof(GLfloat), tex2));

//  const GLushort index[2 * 3] = {
//    0, 1, 2, // Triangle 0
//    1, 3, 2 };
//  index_vbo.reset(new ElementArrayBuffer(4 * 3 * sizeof(GLushort), index));
  const GLushort index[4 * 3] = {
    0, 1, 2, // Triangle 0
    1, 4, 2,
    2, 4, 3,
    0, 2, 3 };
  index_vbo.reset(new ElementArrayBuffer(4 * 3 * sizeof(GLushort), index));

  phong_va.reset(new VertexArray);
  phong_va->bind();

  const Attrib position_attrib = phong->activeAttrib("position");
  const Bindor<ArrayBuffer> position_vbo_bindor(*position_vbo);
  const VertexAttribArrayEnabler position_vaae(position_attrib.location);
  vertexAttribPointer(
    position_attrib.location,
    3,        // Size.
    VertexAttribType<GLfloat>::VALUE,
    GL_FALSE, // Normalize.
    0,        // Stride.
    0); // Read from currently bound VBO.

  const Attrib* tex2_attrib = phong->queryActiveAttrib("tex2");
  const Bindor<ArrayBuffer> tex2_vbo_bindor(*tex2_vbo);
  const VertexAttribArrayEnabler tex2_vaae(tex2_attrib->location);
  vertexAttribPointer(
    tex2_attrib->location,
    2,        // Size.
    VertexAttribType<GLfloat>::VALUE,
    GL_FALSE, // Normalize.
    0,        // Stride.
    0); // Read from currently bound VBO.

  const Bindor<ElementArrayBuffer> index_bindor(*index_vbo);
  phong_va->release();
}

void render()
{
  viewport(0, 0, winWidth, winHeight);
  clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  const Enabler depth_test_enabler(GL_DEPTH_TEST);

  const Bindor<ShaderProgram> phong_bindor(*phong);
  const Bindor<VertexArray> phong_va_bindor(*phong_va);
  drawRangeElements(
    GL_TRIANGLES,
    0,
    static_cast<GLuint>(position_vbo->sizeInBytes() / 3 * sizeof(GLfloat)),
    3 * static_cast<GLsizei>(index_vbo->sizeInBytes() / 3 * sizeof(GLushort)),
    GLTypeEnum<GLushort>::value,
    0); // Read indices from currently bound element array.
}

int main(int argc, char* argv[])
{
  using namespace std;
  using namespace ndj;

  try {

    initGLFW(winWidth, winHeight);
    initGLEW();
    initGL();

    buildShaderPrograms();

    initScene();

    while (!glfwWindowShouldClose(win))
    {
      render();

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
