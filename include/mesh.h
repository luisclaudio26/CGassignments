#ifndef MESH_H
#define MESH_H

#include <string>
#include <vector>
#include <nanogui/glutil.h>
#include "primitives.h"

//the elements of our packed data
struct Elem
{
  Vec3 pos, normal;
  RGB a, d, s;
  float shininess;
};

class Mesh
{
private:
  std::vector<Triangle> tris;
  std::vector<Material> mats;

public:
  Eigen::MatrixXf mPos, mNormal, mAmb, mDiff, mSpec, mShininess;

  Mesh() {}
  Mesh(const std::string& path)
  {
    load_file(path);
  }

  void load_file(const std::string& path);
};

#endif
