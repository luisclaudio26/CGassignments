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

  Eigen::MatrixXf mPos, mNormal, mAmb, mDiff, mSpec;
  Eigen::VectorXf mShininess;

  /*
  Elem *packed_data; int sz_packed_data;
  GLuint VAO, shader;
  void pack_data();
  */

public:
  Mesh() {}
  Mesh(const std::string& path)
  {
    load_file(path);
  }

  //~Mesh() { if(packed_data) delete[] packed_data; }

  void load_file(const std::string& path);
/*void upload_to_gpu(GLuint shader);
  void draw(); */


};

#endif
