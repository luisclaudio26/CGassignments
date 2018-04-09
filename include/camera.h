#ifndef CAMERA_H
#define CAMERA_H

#include <nanogui/glutil.h>

struct Camera
{
  //Camera parameters
  Eigen::Vector3f eye, Look_dir, up, right;
  float near, far, step, FoV;
  bool lock_view;
};

#endif
