#include "../include/almostgl.h"
#include "../include/matrix.h"
#include <glm/gtc/matrix_access.hpp>
#include <nanogui/opengl.h>

AlmostGL::AlmostGL(const GlobalParameters& param,
                    const char* path,
                    Widget *parent) : nanogui::GLCanvas(parent), param(param)
{
  this->model.load_file(path);

  //We'll use the same model matrix for both contexts, as we
  //are loading the same model on both.
  //this->model.transform_to_center(mModel);

  this->shader.initFromFiles("almostgl",
                              "../shaders/almostgl.vs",
                              "../shaders/almostgl.fs");

  this->shader.bind();
  this->shader.uploadAttrib<Eigen::MatrixXf>("pos", this->model.mPos);
  this->shader.uploadAttrib<Eigen::MatrixXf>("normal", this->model.mNormal);
  this->shader.uploadAttrib<Eigen::MatrixXf>("amb", this->model.mAmb);
  this->shader.uploadAttrib<Eigen::MatrixXf>("diff", this->model.mDiff);
  this->shader.uploadAttrib<Eigen::MatrixXf>("spec", this->model.mSpec);
  this->shader.uploadAttrib<Eigen::MatrixXf>("shininess", this->model.mShininess);

  //preallocate buffer where we'll store the transformed vertices
  //before copying them to the GPU
  vbuffer = new float[model.mPos.cols()*4];
}

void AlmostGL::drawGL()
{
  using namespace nanogui;

  //TODO: first stage of graphic pipeline
  //X 1) compute look_at matrix
  //X 2) precompute mvp = proj * view * model
  //X 3) transform each vertex v = mvp * v;
  //4) divide by w
  //5) loop through primitives and discard those outside
  //   the canonical view cube (clipping)
  //6) send the remaining vertices to the passthrough vertex shader

  //convert params to use internal library
  vec3 eye = vec3(param.cam.eye[0], param.cam.eye[1], param.cam.eye[2]);
  vec3 up = vec3(param.cam.up[0], param.cam.up[1], param.cam.up[2]);
  vec3 right = vec3(param.cam.right[0], param.cam.right[1], param.cam.right[2]);
  vec3 look_dir = vec3(param.cam.look_dir[0], param.cam.look_dir[1], param.cam.look_dir[2]);

  mat4 model2world;
  for(int i = 0; i < 4; ++i)
    for(int j = 0; j < 4; ++j)
      model2world(i,j) = param.model2world[j][i];

  //important matrices
  mat4 view = mat4::view(eye, eye+look_dir, up);
  mat4 proj = mat4::perspective(param.cam.FoV, 1.7777f, param.cam.near, param.cam.far);
  mat4 mvp_ = proj * view * model2world;

  //geometric transformations
  for(int v_id = 0; v_id < model.mPos.cols(); ++v_id)
  {
    Eigen::Vector3f v = model.mPos.col(v_id);
    vec4 v_ = mvp_ * vec4(v(0), v(1), v(2), 1.0f);

    //copy to vbuffer
    for(int i = 0; i < 4; ++i)
      vbuffer[4*v_id+i] = v_(i);
  }

  //data uploading
  this->shader.bind();
  this->shader.setUniform("model_color", param.model_color);

  //-- we unfortunately need to use uploadAttrib which will call glBufferData
  //-- under the hood. Using glBufferSubData() won't work.
  Eigen::MatrixXf vbuffer_ = Eigen::Map<Eigen::MatrixXf>(vbuffer, 4, this->model.mPos.cols());
  this->shader.uploadAttrib<Eigen::MatrixXf>("pos", vbuffer_);

  //Z buffering
  glEnable(GL_DEPTH_TEST);

  //Backface/Frontface culling
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(param.front_face);

  //draw mode
  glPolygonMode(GL_FRONT_AND_BACK, param.draw_mode);

  this->shader.drawArray(GL_TRIANGLES, 0, this->model.mPos.cols());

  //disable options
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glDisable(GL_DEPTH_TEST);
}
