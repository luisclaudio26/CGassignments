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

#define THETA 0.0174533f
#define COSTHETA float(cos(THETA))
#define SINTHETA float(sin(THETA))

class ExampleApp : public nanogui::Screen
{
private:
  nanogui::GLShader mShader;
  Mesh mMesh;

  //Camera parameters
  glm::vec3 mEye, mLookDir, mUp, mRight;
  float mNear, mFar, mStep, mFoV;
  bool mLockView;

  //model parameters
  Eigen::Vector4f mModelColor;
  glm::mat4 mModel;

  //general parameters
  GLenum mFrontFace;
  GLenum mDrawMode;

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
    reset_view->setCallback( [this] { mUp = glm::vec3(0.0f, 1.0f, 0.0f);
                                      mLookDir = glm::vec3(0.0f, 0.0f, -1.0f);
                                      mRight = glm::vec3(1.0f, 0.0f, 0.0f);
                                      mEye = glm::vec3(0.0f, 0.0f, 0.0f);
                                      mNear = 1.0f; mFar = 10.0f;
                                      mFoV = 45.0f; });

    new Label(window, "Near Z plane", "sans-bold");

    Slider *near_plane = new Slider(window);
    near_plane->setFixedWidth(100);
    near_plane->setTooltip("Set near Z plane to any value between 0 and 20");
    near_plane->setCallback( [this](float val) { mNear = val * 20.0f; } );

    new Label(window, "Far Z plane", "sans-bold");

    Slider *far_plane = new Slider(window);
    far_plane->setFixedWidth(100);
    far_plane->setTooltip("Set near Z plane to any value between 10 and 100");
    far_plane->setCallback( [this](float val) { mFar = 10.0f + val * (100.0f - 10.0f); } );

    new Label(window, "Field of view (deg)", "sans-bold");

    Slider *fov = new Slider(window);
    fov->setFixedWidth(100);
    fov->setTooltip("Set the field of view to any value between 5 and 150");
    fov->setCallback( [this](float val) { mFoV = 5.0f + val * (150.0f - 5.0f); } );

    new Label(window, "Model color", "sans-bold");
    ColorPicker *color_picker = new ColorPicker(window, mModelColor);
    color_picker->setFinalCallback([this](const Color& c) { this->mModelColor = c; });

    CheckBox *draw_cw = new CheckBox(window, "Draw triangles in CW order");
    draw_cw->setTooltip("Uncheck this box for drawing triangles in CCW order");
    draw_cw->setCallback([&](bool cw) { mFrontFace = cw ? GL_CW : GL_CCW; });

    CheckBox *lock_view = new CheckBox(window, "Lock view on the model");
    lock_view->setTooltip("Lock view point at the point where the model is centered. This will disable camera rotation.");
    lock_view->setCallback([&](bool lock) { mLockView = lock;
                                            mLookDir = glm::normalize(glm::vec3(0.0f, 0.0f, -5.5f) - mEye);
                                            mRight = glm::cross(mLookDir, mUp);
                                          });

    ComboBox *draw_mode = new ComboBox(window, {"Points", "Wireframe", "Fill"});
    draw_mode->setCallback([&](int opt) {
                            switch(opt)
                            {
                              case 0: mDrawMode = GL_POINT; break;
                              case 1: mDrawMode = GL_LINE; break;
                              case 2: mDrawMode = GL_FILL; break;
                            } });

    performLayout();

    //----------------------------------------
    //----------- Geometry loading -----------
    //----------------------------------------
    mMesh.load_file(path);

    mMesh.transform_to_center(mModel);
    mModelColor<<0.0f, 1.0f, 0.0f, 1.0f;

    printf("%s\n", glm::to_string(mModel).c_str());

    mEye = glm::vec3(0.0f, 0.0f, 0.0f);
    mLookDir = glm::vec3(0.0f, 0.0f, -1.0f);
    mUp = glm::vec3(0.0f, 1.0f, 0.0f);
    mRight = glm::cross(mLookDir, mUp);
    mNear = 1.0f; mFar = 10.0f;
    mStep = 0.1f;
    mLockView = false;
    mFoV = 45.0;

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
    mFrontFace = GL_CCW;

    //draw as filled polygons
    mDrawMode = GL_POINTS;
  }

  virtual void draw(NVGcontext *ctx)
  {
    Screen::draw(ctx);
  }

  virtual bool keyboardEvent(int key, int scancode, int action, int modifiers) {
    if (Screen::keyboardEvent(key, scancode, action, modifiers)) return true;

    //camera movement
    if(key == GLFW_KEY_A && action == GLFW_REPEAT) {
      mEye += (-mRight) * mStep;
      if(mLockView)
      {
        mLookDir = glm::normalize(glm::vec3(0.0f, 0.0f, -5.5f) - mEye);
        mRight = glm::cross(mLookDir, mUp);
      }
      return true;
    }
    if(key == GLFW_KEY_D && action == GLFW_REPEAT) {
      mEye += mRight * mStep;
      if(mLockView)
      {
        mLookDir = glm::normalize(glm::vec3(0.0f, 0.0f, -5.5f) - mEye);
        mRight = glm::cross(mLookDir, mUp);
      }
      return true;
    }
    if( key == GLFW_KEY_W && action == GLFW_REPEAT ) {
      mEye += mLookDir * mStep;
      //if(mLockView) mLookDir = glm::normalize(glm::vec3(0.0f, 0.0f, -5.5f) - mEye);
      return true;
    }
    if( key == GLFW_KEY_S && action == GLFW_REPEAT ) {
      mEye += mLookDir * (-mStep);
      //if(mLockView) mLookDir = glm::normalize(glm::vec3(0.0f, 0.0f, -5.5f) - mEye);
      return true;
    }
    if( key == GLFW_KEY_R && action == GLFW_REPEAT ) {
      mEye += mUp * mStep;
      if(mLockView)
      {
        mLookDir = glm::normalize(glm::vec3(0.0f, 0.0f, -5.5f) - mEye);
        mUp = glm::cross(mRight, mLookDir);
      }

      return true;
    }
    if( key == GLFW_KEY_F && action == GLFW_REPEAT ) {
      mEye += (-mUp) * mStep;
      if(mLockView)
      {
        mLookDir = glm::normalize(glm::vec3(0.0f, 0.0f, -5.5f) - mEye);
        mUp = glm::cross(mRight, mLookDir);
      }
      return true;
    }

    //TODO: we can precompute sin and cos values!
    if( key == GLFW_KEY_U && action == GLFW_REPEAT ) {
      if(mLockView) return true;

      glm::vec3 u = mLookDir, v = mUp;
      mLookDir = COSTHETA*u + SINTHETA*v;
      mUp = -SINTHETA*u + COSTHETA*v;

      return true;
    }
    if( key == GLFW_KEY_J && action == GLFW_REPEAT ) {
      if(mLockView) return true;

      glm::vec3 u = mLookDir, v = mUp;
      mLookDir = COSTHETA*u + -SINTHETA*v;
      mUp = SINTHETA*u + COSTHETA*v;

      return true;
    }
    if( key == GLFW_KEY_K && action == GLFW_REPEAT ) {
      if(mLockView) return true;

      glm::vec3 u = mRight, v = mLookDir;
      mRight = COSTHETA*u + -SINTHETA*v;
      mLookDir = SINTHETA*u + COSTHETA*v;

      return true;
    }
    if( key == GLFW_KEY_H && action == GLFW_REPEAT ) {
      if(mLockView) return true;

      glm::vec3 u = mRight, v = mLookDir;
      mRight = COSTHETA*u + SINTHETA*v;
      mLookDir = -SINTHETA*u + COSTHETA*v;

      return true;
    }

    //------------
    if( key == GLFW_KEY_M && action == GLFW_REPEAT ) {
      if(mLockView) return true;

      glm::vec3 u = mRight, v = mUp;
      mRight = COSTHETA*u + -SINTHETA*v;
      mUp = SINTHETA*u + COSTHETA*v;

      return true;
    }
    if( key == GLFW_KEY_N && action == GLFW_REPEAT ) {
      if(mLockView) return true;

      glm::vec3 u = mRight, v = mUp;
      mRight = COSTHETA*u + SINTHETA*v;
      mUp = -SINTHETA*u + COSTHETA*v;

      return true;
    }
    //---------------

    return false;
  }

  virtual void drawContents()
  {
    using namespace nanogui;

    //uniform uploading
    glm::mat4 view = glm::lookAt(mEye, mEye + mLookDir, mUp);
    glm::mat4 proj = glm::perspective(glm::radians(mFoV), 1.6666f, mNear, mFar);

    glm::mat4 mvp_ = proj * view * mModel;
    Eigen::Matrix4f mvp = Eigen::Map<Matrix4f>( glm::value_ptr(mvp_) );

    //actual drawing
    mShader.bind();
    mShader.setUniform("mvp", mvp);
    mShader.setUniform("model_color", mModelColor);

    //Z buffering
    glEnable(GL_DEPTH_TEST);

    //Backface/Frontface culling
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(mFrontFace);

    //draw mode
    glPolygonMode(GL_FRONT_AND_BACK, mDrawMode);

    mShader.drawArray(GL_TRIANGLES, 0, mMesh.mPos.cols());

    //disable options
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_DEPTH_TEST);
  }
};

int main(int argc, char** args)
{
  nanogui::init();

  /* scoped variables. why this? */ {
    nanogui::ref<ExampleApp> app = new ExampleApp(args[1]);
    app->drawAll();
    app->setVisible(true);
    nanogui::mainloop();
  }

  nanogui::shutdown();
}
