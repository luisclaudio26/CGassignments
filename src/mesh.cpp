#include "../include/mesh.h"
#include <cstdio>
#include <glm/gtx/string_cast.hpp>

void Mesh::load_file(const std::string& path)
{
  tris.clear(); mats.clear();
  FILE *file = fopen( path.c_str(), "r");

  //1. name
  char obj_name[100];
  fscanf(file, "Object name = %s\n", obj_name);

  //2. triangle count
  int n_tris;
  fscanf(file, "# triangles = %d\n", &n_tris);
  tris.resize(n_tris);

  //3. material count
  int n_mats;
  fscanf(file, "Material count = %d\n", &n_mats);
  mats.resize(n_mats);


    printf("%d %d\n", n_mats, n_tris);

  //4. materials (groups of 4 lines describing amb, diff, spec and shininess)
  for(int i = 0; i < n_mats; ++i)
  {
    Material& cur = mats[i];
    fscanf(file, "ambient color %f %f %f\n", &cur.a[0], &cur.a[1], &cur.a[2]);
    fscanf(file, "diffuse color %f %f %f\n", &cur.d[0], &cur.d[1], &cur.d[2]);
    fscanf(file, "specular color %f %f %f\n", &cur.s[0], &cur.s[1], &cur.s[2]);
    fscanf(file, "material shine %f\n", &cur.shininess);
  }

  //5. spurious line
  fscanf(file, "-- 3*[pos(x,y,z) normal(x,y,z) color_index] face_normal(x,y,z)\n");

  //6. triangles (groups of 4 lines describing per vertex data and normal)
  for(int i = 0; i < n_tris; ++i)
  {
    Triangle& cur = tris[i];
    fscanf(file, "v0 %f %f %f %f %f %f %d\n", &cur.v[0].pos.x, &cur.v[0].pos.y, &cur.v[0].pos.z,
                                              &cur.v[0].normal.x, &cur.v[0].normal.y, &cur.v[0].normal.z,
                                              &cur.v[0].m_index);

    fscanf(file, "v1 %f %f %f %f %f %f %d\n", &cur.v[1].pos.x, &cur.v[1].pos.y, &cur.v[1].pos.z,
                                              &cur.v[1].normal.x, &cur.v[1].normal.y, &cur.v[1].normal.z,
                                              &cur.v[1].m_index);

    fscanf(file, "v2 %f %f %f %f %f %f %d\n", &cur.v[2].pos.x, &cur.v[2].pos.y, &cur.v[2].pos.z,
                                              &cur.v[2].normal.x, &cur.v[2].normal.y, &cur.v[2].normal.z,
                                              &cur.v[2].m_index);

    fscanf(file, "face normal %f %f %f\n", &cur.normal.x, &cur.normal.y, &cur.normal.z);
  }

  fclose(file);
}
