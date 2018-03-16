#ifndef MESH_H
#define MESH_H

#include <string>
#include <vector>
#include <GL/glew.h>
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

  Elem *packed_data; int sz_packed_data;

  GLuint VAO, shader;

  void pack_data();

public:
  Mesh() {}
  Mesh(const std::string& path) : packed_data(NULL)
  {
    load_file(path);
  }

  ~Mesh()
  {
      if(packed_data) delete[] packed_data;
  }

  void draw();
  void load_file(const std::string& path);
  void upload_to_gpu(GLuint shader);
};

#endif
