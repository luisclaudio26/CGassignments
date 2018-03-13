#ifndef TRIANGLE_H
#define TRIANGLE_H

struct Vertex
{
  float x, y, z;
  float r, g, b;
  //float nx, ny, nz;
  //float u, v;
};

struct Triangle
{
  Vertex v[3];
};

#endif
