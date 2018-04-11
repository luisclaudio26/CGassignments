#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <iostream>

#include <nanogui/opengl.h>
#include <nanogui/glutil.h>
#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <nanogui/layout.h>
#include <nanogui/button.h>
#include <nanogui/slider.h>
#include <nanogui/label.h>
#include <nanogui/checkbox.h>
#include <nanogui/colorpicker.h>
#include <nanogui/combobox.h>

#include "../include/mesh.h"
#include "../include/almostgl.h"
#include "../include/param.h"
#include "../include/matrix.h"

#define THETA 0.0174533f
#define COSTHETA float(cos(THETA))
#define SINTHETA float(sin(THETA))

class ExampleApp : public nanogui::Screen
{
private:
  nanogui::GLShader mShader;
  Mesh mMesh;
  AlmostGL *mCanvas;

  GlobalParameters param;

public:
  ExampleApp(const char* path) : nanogui::Screen(Eigen::Vector2i(960, 540), "NanoGUI Test")
  {
    //----------------------------------
    //----------- GUI setup ------------
    //----------------------------------
    using namespace nanogui;

    Window *window = new Window(this, "Scene options");
    window->setPosition(Vector2i(0, 0));
    window->setLayout(new GroupLayout());

    Button *reset_view = new Button(window, "Reset view");
    reset_view->setTooltip("Reset view so the object will be centered again");
    reset_view->setCallback( [this] { param.cam.up = glm::vec3(0.0f, 1.0f, 0.0f);
                                      param.cam.look_dir = glm::vec3(0.0f, 0.0f, -1.0f);
                                      param.cam.right = glm::vec3(1.0f, 0.0f, 0.0f);
                                      param.cam.eye = glm::vec3(0.0f, 0.0f, 0.0f);
                                      param.cam.near = 1.0f; param.cam.far = 10.0f;
                                      param.cam.FoV = 45.0f; });

    new Label(window, "Near Z plane", "sans-bold");

    Slider *near_plane = new Slider(window);
    near_plane->setFixedWidth(100);
    near_plane->setTooltip("Set near Z plane to any value between 0 and 20");
    near_plane->setCallback( [this](float val) { param.cam.near = val * 20.0f; } );

    new Label(window, "Far Z plane", "sans-bold");

    Slider *far_plane = new Slider(window);
    far_plane->setFixedWidth(100);
    far_plane->setTooltip("Set near Z plane to any value between 10 and 100");
    far_plane->setCallback( [this](float val) { param.cam.far = 10.0f + val * (100.0f - 10.0f); } );

    new Label(window, "Field of view (deg)", "sans-bold");

    Slider *fov = new Slider(window);
    fov->setFixedWidth(100);
    fov->setTooltip("Set the field of view to any value between 5 and 150");
    fov->setCallback( [this](float val) { param.cam.FoV = 5.0f + val * (150.0f - 5.0f); } );

    new Label(window, "Model color", "sans-bold");
    ColorPicker *color_picker = new ColorPicker(window, param.model_color);
    color_picker->setFinalCallback([this](const Color& c) { this->param.model_color = c; });

    CheckBox *draw_cw = new CheckBox(window, "Draw triangles in CW order");
    draw_cw->setTooltip("Uncheck this box for drawing triangles in CCW order");
    draw_cw->setCallback([&](bool cw) { param.front_face = cw ? GL_CW : GL_CCW; });

    CheckBox *lock_view = new CheckBox(window, "Lock view on the model");
    lock_view->setTooltip("Lock view point at the point where the model is centered. This will disable camera rotation.");
    lock_view->setCallback([&](bool lock) { param.cam.lock_view = lock;
                                            param.cam.look_dir = glm::normalize(glm::vec3(0.0f, 0.0f, -5.5f) - param.cam.eye);
                                            param.cam.right = glm::cross(param.cam.look_dir, param.cam.up);
                                          });

    ComboBox *draw_mode = new ComboBox(window, {"Points", "Wireframe", "Fill"});
    draw_mode->setCallback([&](int opt) {
                            switch(opt)
                            {
                              case 0: param.draw_mode = GL_POINT; break;
                              case 1: param.draw_mode = GL_LINE; break;
                              case 2: param.draw_mode = GL_FILL; break;
                            } });

    Window *winAlmostGL = new Window(this, "AlmostGL");
    winAlmostGL->setPosition(Eigen::Vector2i(50,50));
    winAlmostGL->setLayout(new GroupLayout());
    mCanvas = new AlmostGL(param, path, winAlmostGL);

    performLayout();

    //----------------------------------------
    //----------- Geometry loading -----------
    //----------------------------------------
    mMesh.load_file(path);

    mMesh.transform_to_center(param.model2world);
    param.model_color<<0.0f, 1.0f, 0.0f, 1.0f;

    printf("%s\n", glm::to_string(param.model2world).c_str());

    param.cam.eye = glm::vec3(0.0f, 0.0f, 0.0f);
    param.cam.look_dir = glm::vec3(0.0f, 0.0f, -1.0f);
    param.cam.up = glm::vec3(0.0f, 1.0f, 0.0f);
    param.cam.right = glm::vec3(1.0f, 0.0f, 0.0f);
    param.cam.near = 1.0f; param.cam.far = 10.0f;
    param.cam.step = 0.1f;
    param.cam.FoV = 45.0f;
    param.cam.lock_view = false;

    //--------------------------------------
    //----------- Shader options -----------
    //--------------------------------------
    mShader.initFromFiles("passthrough",
                          "../shaders/passthrough.vs",
                          "../shaders/passthrough.fs");

    mShader.bind();
    mShader.uploadAttrib<Eigen::MatrixXf>("pos", mMesh.mPos);
    mShader.uploadAttrib<Eigen::MatrixXf>("normal", mMesh.mNormal);
    mShader.uploadAttrib<Eigen::MatrixXf>("amb", mMesh.mAmb);
    mShader.uploadAttrib<Eigen::MatrixXf>("diff", mMesh.mDiff);
    mShader.uploadAttrib<Eigen::MatrixXf>("spec", mMesh.mSpec);
    mShader.uploadAttrib<Eigen::MatrixXf>("shininess", mMesh.mShininess);

    //front faces are the counter-clockwise ones
    param.front_face = GL_CCW;

    //draw as filled polygons
    param.draw_mode = GL_POINTS;
  }

  virtual void draw(NVGcontext *ctx)
  {
    Screen::draw(ctx);
  }

  virtual bool keyboardEvent(int key, int scancode, int action, int modifiers) {
    if (Screen::keyboardEvent(key, scancode, action, modifiers)) return true;

    //camera movement
    if(key == GLFW_KEY_A && action == GLFW_REPEAT) {
      param.cam.eye += (-param.cam.right) * param.cam.step;
      if(param.cam.lock_view)
      {
        param.cam.look_dir = glm::normalize(glm::vec3(0.0f, 0.0f, -5.5f) - param.cam.eye);
        param.cam.right = glm::cross(param.cam.look_dir, param.cam.up);
      }
      return true;
    }
    if(key == GLFW_KEY_D && action == GLFW_REPEAT) {
      param.cam.eye += param.cam.right * param.cam.step;
      if(param.cam.lock_view)
      {
        param.cam.look_dir = glm::normalize(glm::vec3(0.0f, 0.0f, -5.5f) - param.cam.eye);
        param.cam.right = glm::cross(param.cam.look_dir, param.cam.up);
      }
      return true;
    }
    if( key == GLFW_KEY_W && action == GLFW_REPEAT ) {
      param.cam.eye += param.cam.look_dir * param.cam.step;
      //if(param.cam.lock_view) param.cam.look_dir = glm::normalize(glm::vec3(0.0f, 0.0f, -5.5f) - param.cam.eye);
      return true;
    }
    if( key == GLFW_KEY_S && action == GLFW_REPEAT ) {
      param.cam.eye += param.cam.look_dir * (-param.cam.step);
      //if(param.cam.lock_view) param.cam.look_dir = glm::normalize(glm::vec3(0.0f, 0.0f, -5.5f) - param.cam.eye);
      return true;
    }
    if( key == GLFW_KEY_R && action == GLFW_REPEAT ) {
      param.cam.eye += param.cam.up * param.cam.step;
      if(param.cam.lock_view)
      {
        param.cam.look_dir = glm::normalize(glm::vec3(0.0f, 0.0f, -5.5f) - param.cam.eye);
        param.cam.up = glm::cross(param.cam.right, param.cam.look_dir);
      }

      return true;
    }
    if( key == GLFW_KEY_F && action == GLFW_REPEAT ) {
      param.cam.eye += (-param.cam.up) * param.cam.step;
      if(param.cam.lock_view)
      {
        param.cam.look_dir = glm::normalize(glm::vec3(0.0f, 0.0f, -5.5f) - param.cam.eye);
        param.cam.up = glm::cross(param.cam.right, param.cam.look_dir);
      }
      return true;
    }

    //TODO: we can precompute sin and cos values!
    if( key == GLFW_KEY_U && action == GLFW_REPEAT ) {
      if(param.cam.lock_view) return true;

      glm::vec3 u = param.cam.look_dir, v = param.cam.up;
      param.cam.look_dir = COSTHETA*u + SINTHETA*v;
      param.cam.up = -SINTHETA*u + COSTHETA*v;

      return true;
    }
    if( key == GLFW_KEY_J && action == GLFW_REPEAT ) {
      if(param.cam.lock_view) return true;

      glm::vec3 u = param.cam.look_dir, v = param.cam.up;
      param.cam.look_dir = COSTHETA*u + -SINTHETA*v;
      param.cam.up = SINTHETA*u + COSTHETA*v;

      return true;
    }
    if( key == GLFW_KEY_K && action == GLFW_REPEAT ) {
      if(param.cam.lock_view) return true;

      glm::vec3 u = param.cam.right, v = param.cam.look_dir;
      param.cam.right = COSTHETA*u + -SINTHETA*v;
      param.cam.look_dir = SINTHETA*u + COSTHETA*v;

      return true;
    }
    if( key == GLFW_KEY_H && action == GLFW_REPEAT ) {
      if(param.cam.lock_view) return true;

      glm::vec3 u = param.cam.right, v = param.cam.look_dir;
      param.cam.right = COSTHETA*u + SINTHETA*v;
      param.cam.look_dir = -SINTHETA*u + COSTHETA*v;

      return true;
    }

    //------------
    if( key == GLFW_KEY_M && action == GLFW_REPEAT ) {
      if(param.cam.lock_view) return true;

      glm::vec3 u = param.cam.right, v = param.cam.up;
      param.cam.right = COSTHETA*u + -SINTHETA*v;
      param.cam.up = SINTHETA*u + COSTHETA*v;

      return true;
    }
    if( key == GLFW_KEY_N && action == GLFW_REPEAT ) {
      if(param.cam.lock_view) return true;

      glm::vec3 u = param.cam.right, v = param.cam.up;
      param.cam.right = COSTHETA*u + SINTHETA*v;
      param.cam.up = -SINTHETA*u + COSTHETA*v;

      return true;
    }
    //---------------

    return false;
  }

  virtual void drawContents()
  {
    using namespace nanogui;

    //uniform uploading
    glm::mat4 view = glm::lookAt(param.cam.eye,
                                  param.cam.eye + param.cam.look_dir,
                                  param.cam.up);

    glm::mat4 proj = glm::perspective(glm::radians(param.cam.FoV),
                                      1.7777f,
                                      param.cam.near, param.cam.far);

    glm::mat4 mvp_ = proj * view * param.model2world;
    Eigen::Matrix4f mvp = Eigen::Map<Matrix4f>( glm::value_ptr(mvp_) );

    //actual drawing
    mShader.bind();
    mShader.setUniform("mvp", mvp);
    mShader.setUniform("model_color", param.model_color);

    //Z buffering
    glEnable(GL_DEPTH_TEST);

    //Backface/Frontface culling
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(param.front_face);

    //draw mode
    glPolygonMode(GL_FRONT_AND_BACK, param.draw_mode);

    mShader.drawArray(GL_TRIANGLES, 0, mMesh.mPos.cols());

    //disable options
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_DEPTH_TEST);
  }
};

int main(int argc, char** args)
{
  mat4 p = mat4::perspective(60.0f, 1.7777f, 2.0f, 10.0f);

  for(int i = 0; i < 4; ++i)
  {
    for(int j = 0; j < 4; ++j)
      printf("%f ", p(i,j));
    printf("\n");
  }

  glm::mat4 p_ = glm::perspective(glm::radians(60.0f), 1.7777f, 2.0f, 10.0f);
  printf("\n\n%s\n", glm::to_string(p_).c_str());

  nanogui::init();

  /* scoped variables. why this? */ {
    nanogui::ref<ExampleApp> app = new ExampleApp(args[1]);
    app->drawAll();
    app->setVisible(true);
    nanogui::mainloop();
  }

  nanogui::shutdown();
}
