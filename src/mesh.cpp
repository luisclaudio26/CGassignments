#include "../include/mesh.h"
#include <cstdio>
#include <glm/gtx/string_cast.hpp>

#define GL_OK { GLenum err; \
                if( (err = glGetError()) != GL_NO_ERROR) \
                  printf("Error at %s, %d: %d\n", __FILE__, __LINE__, err); \
              }



static void define_float_attrib(GLuint shader, const char* name,
                                int size, int stride, int offset,
                                bool normalized = true)
{
  GLuint attrib = glGetAttribLocation(shader, name); GL_OK
  glEnableVertexAttribArray(attrib); GL_OK
  glVertexAttribPointer(attrib,
                        size,
                        GL_FLOAT,
                        normalized ? GL_FALSE : GL_TRUE,
                        stride,
                        (GLvoid*)(offset*sizeof(float))); GL_OK
}

//-----------------------------------------
//------------- FROM MESH.H ---------------
//-----------------------------------------

void Mesh::draw()
{
  glUseProgram(this->shader); GL_OK
  glBindVertexArray(this->VAO); GL_OK
  glDrawArrays(GL_TRIANGLES, 0, sz_packed_data); GL_OK
  glBindVertexArray(0);
  glUseProgram(0);
}

void Mesh::upload_to_gpu(GLuint shader)
{
  //pack mesh data into a single array to be uploaded to the GPU
  pack_data();

  this->shader = shader;

  glGenVertexArrays(1, &this->VAO); GL_OK
  GLuint VBO; glGenBuffers(1, &VBO); GL_OK

  //upload data
  glBindVertexArray(this->VAO); GL_OK
  glBindBuffer(GL_ARRAY_BUFFER, VBO); GL_OK

  glBufferData(GL_ARRAY_BUFFER,
                sz_packed_data * sizeof(Elem),
                (GLvoid*)packed_data,
                GL_STATIC_DRAW); GL_OK

  printf("Uploading %d bytes\n", sz_packed_data * sizeof(Elem));

  //define attributes
  define_float_attrib(shader, "pos", 3, sizeof(Elem), 0);
  define_float_attrib(shader, "normal", 3, sizeof(Elem), 3);
  define_float_attrib(shader, "amb", 3, sizeof(Elem), 6);
  define_float_attrib(shader, "diff", 3, sizeof(Elem), 9);
  define_float_attrib(shader, "spec", 3, sizeof(Elem), 12);
  define_float_attrib(shader, "shininess", 1, sizeof(Elem), 15);

  glBindBuffer(GL_ARRAY_BUFFER, 0); glBindVertexArray(0);
}

void Mesh::pack_data()
{
  //skip if we have no mesh data
  if( tris.empty() || mats.empty() ) return;

  this->sz_packed_data = 3 * tris.size();
  this->packed_data = new Elem[this->sz_packed_data];

  for(int i = 0; i < tris.size(); ++i)
    for(int j = 0; j < 3; ++j)
    {
      Vertex& v = tris[i].v[j];
      Material& m = mats[v.m_index];
      Elem& e = this->packed_data[i*3+j];

      e.pos = v.pos;
      e.normal = v.normal;
      e.a = m.a; e.d = m.d; e.s = m.s;
      e.shininess = m.shininess;
    }
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
  tris.resize(n_tris);

  //3. material count
  int n_mats;
  fscanf(file, "Material count = %d\n", &n_mats);
  mats.resize(n_mats);

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
