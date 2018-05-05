#ifndef OGL_H
#define OGL_H

#include <nanogui/glutil.h>
#include <nanogui/glcanvas.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include "../include/mesh.h"
#include "../include/param.h"

class OGL : public nanogui::GLCanvas
{
private:
  nanogui::GLShader shader;
  Mesh model;

  //rigorously this should be a const
  //reference, but had problems with
  //Eigen::Map and this will be hotfix for it
  GlobalParameters& param;

public:
  OGL(GlobalParameters& param,
      const char* path,
      Widget *parent);

  float framerate;

  void drawGL() override;
};

#endif
