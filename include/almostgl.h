#ifndef ALMOST_GL
#define ALMOST_GL

#include <nanogui/glutil.h>
#include <nanogui/glcanvas.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include "../include/mesh.h"
#include "../include/param.h"

typedef unsigned char uchar;

class AlmostGL : public nanogui::GLCanvas
{
private:
  nanogui::GLShader shader;
  Mesh model;

  //vertex buffers
  int n_vertices, vertex_sz;
  float *vbuffer, *clipped, *culled, *projected;

  //pixel buffers
  int buffer_height, buffer_width;
  GLubyte *color; float *depth;

  GLuint color_gpu;

  const GlobalParameters& param;

public:
  AlmostGL(const GlobalParameters& param,
            const char* path,
            Widget *parent);
  ~AlmostGL() { delete[] vbuffer, clipped, culled, projected; }

  float framerate;

  void drawGL() override;
};

#endif
