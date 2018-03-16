#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../include/shaderloader.h"
#include "../include/mesh.h"

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
  Mesh mesh("../data/cube.in");
  mesh.upload_to_gpu(shader);

  glm::mat4 model, view, proj;
  model = glm::mat4(1.0f);
  view = glm::lookAt( Vec3(0.0f, 0.0f, 0.0f),
                      Vec3(0.0f, 0.0f, -1.0f),
                      Vec3(0.0f, 1.0f, 0.0f) );
  proj = glm::perspective(60.0f, 1.6666f, 0.5f, 10.0f);

  glm::mat4 mvp = proj*view*model;
  GLuint mvp_id = glGetUniformLocation(shader, "mvp"); GL_OK

	//------- main loop --------
	do
	{
		//manage keyboard input
		//if( glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS )
		//if( glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS )

		//Clear screen -> this function also clears stencil and depth buffer
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    //upload Model-View-Project matrix
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
