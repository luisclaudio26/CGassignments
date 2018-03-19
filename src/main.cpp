#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

#include <nanogui/opengl.h>
#include <nanogui/glutil.h>
#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <nanogui/layout.h>
#include <nanogui/button.h>
#include <nanogui/slider.h>

#include "../include/mesh.h"

class ExampleApp : public nanogui::Screen
{
private:
  nanogui::GLShader mShader;
  Mesh mMesh;

  /*
  Eigen::Vector3f mEye, mLookDir, mUp;
  eye<<0.0f, 0.0f, 1.0f;
  look_dir<<0.0f, 0.0f,-1.0f;
  up<<0.0f, 1.0f, 0.0f;
  */

  glm::vec3 mEye, mLookDir, mUp;
  glm::mat4 mProj;

public:
  ExampleApp() : nanogui::Screen(Eigen::Vector2i(960, 540), "NanoGUI Test")
  {
    //----------------------------------
    //----------- GUI setup ------------
    //----------------------------------
    using namespace nanogui;

    Window *window = new Window(this, "General options");
    window->setPosition(Vector2i(0, 0));
    window->setLayout(new GroupLayout());

    Button *reset_view = new Button(window, "Reset view");
    reset_view->setCallback( [] { printf("Reset\n"); } );
    reset_view->setTooltip("Reset view so the object will be centered again");

    Slider *near_plane = new Slider(window);
    near_plane->setFixedWidth(100);
    near_plane->setFinalCallback( [](float val) { printf("Final value: %f\n", val); } );

    performLayout();

    //----------------------------------------
    //----------- Geometry loading -----------
    //----------------------------------------
    mMesh.load_file("../data/cube.in");

    mEye = glm::vec3(0.0f, 0.0f, 1.0f);
    mLookDir = glm::vec3(0.0f, 0.0f, -1.0f);
    mUp = glm::vec3(0.0f, 1.0f, 0.0f);
    mProj = glm::perspective(84.0f, 1.6666f, 1.0f, 10.0f);

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
  }

  virtual void draw(NVGcontext *ctx)
  {
    Screen::draw(ctx);
  }

  virtual void drawContents()
  {
    using namespace nanogui;

    glm::mat4 view;
    view = glm::lookAt(mEye, mEye + mLookDir, mUp);
    glm::mat4 mvp_ = mProj * view;
    Eigen::Matrix4f mvp = Eigen::Map<Matrix4f>( glm::value_ptr(mvp_), 4, 4);

    mShader.bind();
    mShader.setUniform("mvp", mvp);
    mShader.drawArray(GL_TRIANGLES, 0, 36);
  }
};

int main(int argc, char** args)
{
  nanogui::init();

  nanogui::ref<ExampleApp> app = new ExampleApp;
  app->drawAll();
  app->setVisible(true);
  nanogui::mainloop();

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
