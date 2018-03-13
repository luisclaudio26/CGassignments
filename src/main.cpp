#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "../include/shaderloader.h"
#include "../include/primitives.h"

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
  Triangle tri;
  tri.v[0] = (Vertex){-0.5, -0.5, 1.0, 1.0, 0.0, 0.0};
  tri.v[1] = (Vertex){+0.5, -0.5, 1.0, 0.0, 1.0, 0.0};
  tri.v[2] = (Vertex){0.0, +0.5, 1.0, 0.0, 0.0, 1.0};

  GLuint VAO, VBO;
  glGenVertexArrays(1, &VAO); GL_OK
  glGenBuffers(1, &VBO); GL_OK

  //upload data
  glBindVertexArray(VAO); GL_OK
  glBindBuffer(GL_ARRAY_BUFFER, VBO); GL_OK

  glBufferData(GL_ARRAY_BUFFER,
                sizeof(tri),
                (GLvoid*)&tri,
                GL_STATIC_DRAW); GL_OK

  //define attributes
  //glVertexAttribPointer


	//------- main loop --------
	do
	{
		//manage keyboard input
		//if( glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS )
		//if( glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS )

		//Clear screen -> this function also clears stencil and depth buffer
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

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
	GLFWwindow *window = glfwCreateWindow(800, 600, "AlmostGL", NULL, NULL);
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
