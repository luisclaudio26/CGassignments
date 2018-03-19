#include "../include/mesh.h"
#include <cstdio>
#include <iostream>
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
  mPos = Eigen::MatrixXf(3, 3*n_tris);
  mNormal = Eigen::MatrixXf(3, 3*n_tris);
  mAmb = Eigen::MatrixXf(3, 3*n_tris);
  mDiff = Eigen::MatrixXf(3, 3*n_tris);
  mSpec = Eigen::MatrixXf(3, 3*n_tris);
  mShininess = Eigen::MatrixXf(1, 3*n_tris);

  //3. material count
  int n_mats;
  fscanf(file, "Material count = %d\n", &n_mats);
  std::vector<Material> mats_buffer; mats_buffer.resize(n_mats);

  //4. materials (groups of 4 lines describing amb, diff, spec and shininess)
  for(int i = 0; i < n_mats; ++i)
  {
    Material& cur = mats_buffer[i];
    fscanf(file, "ambient color %f %f %f\n", &cur.a[0], &cur.a[1], &cur.a[2]);
    fscanf(file, "diffuse color %f %f %f\n", &cur.d[0], &cur.d[1], &cur.d[2]);
    fscanf(file, "specular color %f %f %f\n", &cur.s[0], &cur.s[1], &cur.s[2]);
    fscanf(file, "material shine %f\n", &cur.shininess);
  }

  //5. spurious line
  fscanf(file, "-- 3*[pos(x,y,z) normal(x,y,z) color_index] face_normal(x,y,z)\n");

  //6. triangles (groups of 4 lines describing per vertex data and normal)
  int tri_index = 0;
  for(int i = 0; i < n_tris; ++i)
  {
    //Triangle& cur = tris[i];
    int m_index_v0;
    fscanf(file, "v0 %f %f %f %f %f %f %d\n", &mPos(0, tri_index+0), &mPos(1, tri_index+0), &mPos(2, tri_index+0),
                                              &mNormal(0, tri_index+0), &mNormal(1, tri_index+0), &mNormal(2, tri_index+0),
                                              &m_index_v0);

    Material mv0 = mats_buffer[m_index_v0];
    mAmb.col(tri_index)<<mv0.a[0], mv0.a[1], mv0.a[2];
    mDiff.col(tri_index)<<mv0.d[0], mv0.d[1], mv0.d[2];
    mSpec.col(tri_index)<<mv0.s[0], mv0.s[1], mv0.s[2];
    mShininess.col(tri_index)<<mv0.shininess;

    int m_index_v1;
    fscanf(file, "v1 %f %f %f %f %f %f %d\n", &mPos(0, tri_index+1), &mPos(1, tri_index+1), &mPos(2, tri_index+1),
                                              &mNormal(0, tri_index+1), &mNormal(1, tri_index+1), &mNormal(2, tri_index+1),
                                              &m_index_v1);

    Material mv1 = mats_buffer[m_index_v1];
    mAmb.col(tri_index+1)<<mv1.a[0], mv1.a[1], mv1.a[2];
    mDiff.col(tri_index+1)<<mv1.d[0], mv1.d[1], mv1.d[2];
    mSpec.col(tri_index+1)<<mv1.s[0], mv1.s[1], mv1.s[2];
    mShininess.col(tri_index+1)<<mv1.shininess;

    int m_index_v2;
    fscanf(file, "v2 %f %f %f %f %f %f %d\n", &mPos(0, tri_index+2), &mPos(1, tri_index+2), &mPos(2, tri_index+2),
                                              &mNormal(0, tri_index+2), &mNormal(1, tri_index+2), &mNormal(2, tri_index+2),
                                              &m_index_v2);

    Material mv2 = mats_buffer[m_index_v2];
    mAmb.col(tri_index+2)<<mv2.a[0], mv2.a[1], mv2.a[2];
    mDiff.col(tri_index+2)<<mv2.d[0], mv2.d[1], mv2.d[2];
    mSpec.col(tri_index+2)<<mv2.s[0], mv2.s[1], mv2.s[2];
    mShininess.col(tri_index+2)<<mv2.shininess;

    float fn1, fn2, fn3;
    fscanf(file, "face normal %f %f %f\n", &fn1, &fn2, &fn3);

    tri_index += 3;
  }

  fclose(file);
}
