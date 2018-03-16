#ifndef TRIANGLE_H
#define TRIANGLE_H

#include <glm/glm.hpp>

typedef glm::vec3 Vec3;
typedef glm::vec3 RGB;

struct Vertex
{
  Vec3 pos, normal;
  int m_index;
};

struct Material
{
  RGB a, d, s;
  float shininess;
};

struct Triangle
{
  Vertex v[3];
  Vec3 normal;
};

#endif
