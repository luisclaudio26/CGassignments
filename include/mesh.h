#ifndef MESH_H
#define MESH_H

#include <string>
#include <vector>
#include "primitives.h"

class Mesh
{
private:
  std::vector<Triangle> tris;
  std::vector<Material> mats;

public:
  Mesh() {}
  Mesh(const std::string& path) { load_file(path); }

  void load_file(const std::string& path);
};

#endif
