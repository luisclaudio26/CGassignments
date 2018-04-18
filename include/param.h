#ifndef CAMERA_H
#define CAMERA_H

#include <nanogui/opengl.h>
#include <nanogui/glutil.h>
#include <glm/glm.hpp>

struct Camera
{
  //Camera parameters
  glm::vec3 eye, look_dir, up, right;
  float near, far, step, FoVy, FoVx;
  bool lock_view;
};

struct GlobalParameters
{
  Camera cam;

  //model parameters
  Eigen::Vector3f model_color;
  glm::mat4 model2world;

  //general parameters
  GLenum front_face;
  GLenum draw_mode;
  int shading;
};

#endif
