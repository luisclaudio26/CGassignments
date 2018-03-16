#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "../include/shaderloader.h"
#include "mesh.h"

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


  Triangle tri;
  tri.v[0] = (Vertex){Vec3(-0.5, -0.5, 0.0), Vec3(1.0, 0.0, 0.0)};
  tri.v[1] = (Vertex){Vec3(+0.5, -0.5, 0.0), Vec3(0.0, 1.0, 0.0)};
  tri.v[2] = (Vertex){Vec3(0.0, +0.5, 0.0), Vec3(0.0, 0.0, 1.0)};

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

  printf("Uploading %d bytes\n", sizeof(tri));

  //define attributes
  GLuint pos = glGetAttribLocation(shader, "pos"); GL_OK
  glEnableVertexAttribArray(pos); GL_OK
  glVertexAttribPointer(pos,
                        3,
                        GL_FLOAT,
                        GL_FALSE,
                        sizeof(Vertex),
                        (GLvoid*)0); GL_OK

  GLuint color = glGetAttribLocation(shader, "color"); GL_OK
  glEnableVertexAttribArray(color); GL_OK
  glVertexAttribPointer(color,
                        3,
                        GL_FLOAT,
                        GL_FALSE,
                        sizeof(Vertex),
                        (GLvoid*)(3*sizeof(float))); GL_OK

  glBindBuffer(GL_ARRAY_BUFFER, 0); glBindVertexArray(0);

	//------- main loop --------
	do
	{
		//manage keyboard input
		//if( glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS )
		//if( glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS )

		//Clear screen -> this function also clears stencil and depth buffer
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    //draw our cute triangle
    glUseProgram(shader);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glUseProgram(0);

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
