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

#include "../include/mesh.h"

class ExampleApp : public nanogui::Screen
{
private:
  nanogui::GLShader mShader;
  Mesh mMesh;

  //Camera parameters
  glm::vec3 mEye, mLookDir, mUp;
  float mNear, mFar, mStep;
  bool mLockView;

  //model parameters
  Eigen::Vector4f mModelColor;
  glm::mat4 mModel;

  //general parameters
  GLenum mFrontFace;

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
                                      mEye = glm::vec3(0.0f, 0.0f, 0.0f);
                                      mNear = 1.0f; mFar = 10.0f; });

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

    new Label(window, "Model color", "sans-bold");
    ColorPicker *color_picker = new ColorPicker(window, mModelColor);
    color_picker->setFinalCallback([this](const Color& c) { this->mModelColor = c; });

    CheckBox *draw_cw = new CheckBox(window, "Draw triangles in CW order");
    draw_cw->setTooltip("Uncheck this box for drawing triangles in CCW order");
    draw_cw->setCallback([&](bool cw) { mFrontFace = cw ? GL_CW : GL_CCW; });

    CheckBox *lock_view = new CheckBox(window, "Lock view on the model");
    lock_view->setTooltip("Lock view point at the point where the model is centered. This will disable camera rotation.");
    lock_view->setCallback([&](bool lock) { mLockView = lock;
                                            mLookDir = glm::normalize(glm::vec3(0.0f, 0.0f, -5.5f) - mEye); });

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
    mNear = 1.0f; mFar = 10.0f;
    mStep = 0.1f;
    mLockView = false;

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
  }

  virtual void draw(NVGcontext *ctx)
  {
    Screen::draw(ctx);
  }

  virtual bool keyboardEvent(int key, int scancode, int action, int modifiers) {
    if (Screen::keyboardEvent(key, scancode, action, modifiers)) return true;

    //camera movement
    if(key == GLFW_KEY_A && action == GLFW_REPEAT) {
      glm::vec3 left = glm::cross(mUp, mLookDir);
      mEye += left * mStep;
      if(mLockView) mLookDir = glm::normalize(glm::vec3(0.0f, 0.0f, -5.5f) - mEye);
      return true;
    }
    if(key == GLFW_KEY_D && action == GLFW_REPEAT) {
      glm::vec3 left = glm::cross(mUp, mLookDir);
      mEye -= left * mStep;
      if(mLockView) mLookDir = glm::normalize(glm::vec3(0.0f, 0.0f, -5.5f) - mEye);
      return true;
    }
    if( key == GLFW_KEY_W && action == GLFW_REPEAT ) {
      mEye += mLookDir * mStep;
      if(mLockView) mLookDir = glm::normalize(glm::vec3(0.0f, 0.0f, -5.5f) - mEye);
      return true;
    }
    if( key == GLFW_KEY_S && action == GLFW_REPEAT ) {
      mEye += mLookDir * (-mStep);
      if(mLockView) mLookDir = glm::normalize(glm::vec3(0.0f, 0.0f, -5.5f) - mEye);
      return true;
    }
    if( key == GLFW_KEY_R && action == GLFW_REPEAT ) {
      mEye += mUp * mStep;
      if(mLockView)
      {
        glm::vec3 left = glm::cross(mUp, mLookDir);
        mLookDir = glm::normalize(glm::vec3(0.0f, 0.0f, -5.5f) - mEye);
        mUp = glm::cross(mLookDir, left);
      }

      return true;
    }
    if( key == GLFW_KEY_F && action == GLFW_REPEAT ) {
      mEye += mUp * (-mStep);
      if(mLockView)
      {
        glm::vec3 left = glm::cross(mUp, mLookDir);
        mLookDir = glm::normalize(glm::vec3(0.0f, 0.0f, -5.5f) - mEye);
        mUp = glm::cross(mLookDir, left);
      }
      return true;
    }

    //TODO: we can precompute sin and cos values!
    if( key == GLFW_KEY_UP && action == GLFW_REPEAT ) {
      if(mLockView) return true;

      float theta = 0.0174533f; //1° degree in radians
      float cosTheta = cos(theta), sinTheta = sin(theta);
      glm::vec3 u = mLookDir, v = mUp;

      mLookDir = cosTheta*u + sinTheta*v;
      mUp = -sinTheta*u + cosTheta*v;

      return true;
    }
    if( key == GLFW_KEY_DOWN && action == GLFW_REPEAT ) {
      if(mLockView) return true;
      float theta = -0.0174533f; //1° degree in radians
      float cosTheta = cos(theta), sinTheta = sin(theta);
      glm::vec3 u = mLookDir, v = mUp;

      mLookDir = cosTheta*u + sinTheta*v;
      mUp = -sinTheta*u + cosTheta*v;

      return true;
    }
    if( key == GLFW_KEY_RIGHT && action == GLFW_REPEAT ) {
      if(mLockView) return true;
      float theta = 0.0174533f; //1° degree in radians
      float cosTheta = cos(theta), sinTheta = sin(theta);
      glm::vec3 u = glm::cross(mUp, mLookDir), v = mLookDir;

      mLookDir = -sinTheta*u + cosTheta*v;

      return true;
    }
    if( key == GLFW_KEY_LEFT && action == GLFW_REPEAT ) {
      if(mLockView) return true;
      float theta = -0.0174533f; //1° degree in radians
      float cosTheta = cos(theta), sinTheta = sin(theta);
      glm::vec3 u = glm::cross(mUp, mLookDir), v = mLookDir;

      mLookDir = -sinTheta*u + cosTheta*v;

      return true;
    }

    return false;
  }

  virtual void drawContents()
  {
    using namespace nanogui;

    //uniform uploading
    glm::mat4 view = glm::lookAt(mEye, mEye + mLookDir, mUp);
    glm::mat4 proj = glm::perspective(45.0f, 1.6666f, mNear, mFar);

    glm::mat4 mvp_ = proj * view * mModel;
    Eigen::Matrix4f mvp = Eigen::Map<Matrix4f>( glm::value_ptr(mvp_) );

    //actual drawing
    mShader.bind();
    mShader.setUniform("mvp", mvp);
    mShader.setUniform("model_color", mModelColor);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(mFrontFace);

    mShader.drawArray(GL_TRIANGLES, 0, mMesh.mPos.cols());

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








//-----------------------------------------------------

/*
#define GL_OK { GLenum err; \
                if( (err = glGetError()) != GL_NO_ERROR) \
                  printf("Error at %d: %d\n", __LINE__, err); \
              }

GLFWwindow* setup();

int main(int argc, char** args)
{
	GLFWwindow* window = setup();
	glGetError(); //clean glewInit() error

  //------- shader loading -------
  GLuint shader = ShaderLoader::load("../shaders/passthrough");

  //------- geometry setup -------
  Mesh mesh(args[1]);
  mesh.upload_to_gpu(shader);

  glm::mat4 model = glm::mat4(1.0f);

  //------- camera setup -------
  glm::vec3 eye(0.0f, 0.0f, 2.0f);
  glm::vec3 up(0.0f, 1.0f, 0.0f);
  glm::vec3 look_dir(0.0f, 0.0f, -1.0f);
  float step = 0.05f;

  glm::mat4 proj = glm::perspective(84.0f, 1.6666f, 1.0f, 10.0f);

  GLuint mvp_id = glGetUniformLocation(shader, "mvp"); GL_OK

	//------- main loop --------
	do
	{
    //Clear screen -> this function also clears stencil and depth buffer
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		//manage keyboard input
		if( glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS )
    {
      glm::vec3 left = glm::cross(up, look_dir);
      eye += left * step;
    }
		if( glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS )
    {
      glm::vec3 right = -glm::cross(up, look_dir);
      eye += right * step;
    }
    if( glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ) eye += look_dir * step;
    if( glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ) eye += look_dir * (-step);
    if( glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS ) eye += up * step;
    if( glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS ) eye += up * (-step);

    //compute view matrix
    glm::mat4 view = glm::lookAt(eye, eye+look_dir, up);

    //upload Model-View-Project matrix
    glm::mat4 mvp = proj * view * model;
    glUseProgram(shader);
    glUniformMatrix4fv(mvp_id, 1, GL_FALSE, glm::value_ptr(mvp) ); GL_OK

    //draw our cute triangle
    mesh.draw();

		//Swap buffer and query events
		glfwSwapBuffers(window);
		glfwPollEvents();
	} while(glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
			!glfwWindowShouldClose(window));

	//Drop GLFW device
	glfwTerminate();

	return 0;
}

GLFWwindow* setup()
{
	//This will setup a new window and OpenGL context.
	//GLFW or GLUT may be used for this kind of stuff.
	glfwInit();

	glfwWindowHint(GLFW_SAMPLES, 1); //Supersampling, 4 samples/pixel
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); //Make sure OpenGL
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2); //version is 3.0

	//Something about which kind of OpenGL will run here
	//This says to glfw that we're using only OpenGL core functions, not
	//vendor-specific
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	//Create window and set it as the context
	GLFWwindow *window = glfwCreateWindow(960, 540, "AlmostGL", NULL, NULL);
	glfwMakeContextCurrent(window);

	//Initialize GLEW.
	glewExperimental = GL_TRUE; //Where the fuck is this coming from?
	glewInit(); //this launches an error!

	//Get keyboard input
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	//Enable z-buffering
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	return window;
}
*/
