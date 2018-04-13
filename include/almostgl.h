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

class AlmostGL : public nanogui::GLCanvas
{
private:
  nanogui::GLShader shader;
  Mesh model;

  float *vbuffer;

  const GlobalParameters& param;

public:
  AlmostGL(const GlobalParameters& param,
            const char* path,
            Widget *parent);
  ~AlmostGL() { delete[] vbuffer; }

  void drawGL() override;
};

#endif
