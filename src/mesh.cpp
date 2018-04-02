#include "../include/mesh.h"
#include <cstdio>
#include <iostream>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp>

void Mesh::transform_to_center(glm::mat4& M)
{
  //compute bounding box for this mesh
  glm::vec3 min, max;
  for(int i = 0; i < mPos.cols(); ++i)
  {
    Eigen::Vector3f p = mPos.col(i);
    for(int j = 0; j < 3; ++j)
    {
      min[j] = std::min( min[j], p(j) );
      max[j] = std::max( max[j], p(j) );
    }
  }

  //compute center of the bounding box.
  //this will be used to calculate the
  //translation to the center.
  glm::vec3 center = (min+max)*0.5f;

  //compute translation from the center to
  //the midpoint in the Z axis between the
  //near and far plane. Also, compute scaling
  //so that the objects bounding box has the
  //width as the frutum in the Z plane
  //THIS ASSUMES THE FOLLOWING CAMERA PARAMETERS:
  // - eye = origin,
  // - look_dir = (0,0,-1),
  // - up = (0,1,0),
  // - near = 1
  // - far = 10
  // - FoV = 45°
  glm::mat4 to_origin = glm::translate(glm::mat4(1.0f), glm::vec3(-center.x,
                                                                  -center.y,
                                                                  -center.z));

  glm::vec4 centered_max = M*glm::vec4(max, 1.0f);
  float half_frustrum = 1.0f; //Near x tan(45)
  glm::mat4 scale = glm::scale(glm::mat4(1.0f),
                               glm::vec3(half_frustrum/centered_max.x));

  glm::mat4 from_origin = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -5.5f));

  //final transformation
  M = from_origin * scale * to_origin;
}

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
