#include "../include/almostgl.h"


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
}

void AlmostGL::drawGL()
{
  using namespace nanogui;

  //TODO: first stage of graphic pipeline
  //1) compute look_at matrix
  //2) precompute mvp = proj * view * model
  //3) transform each vertex v = mvp * v;
  //4) divide by w
  //5) loop through primitives and discard those outside
  //   the canonical view cube (clipping)
  //6) send the remaining vertices to the passthrough vertex shader

  //uniform uploading
  glm::mat4 view = glm::lookAt(param.cam.eye,
                                param.cam.eye + param.cam.look_dir,
                                param.cam.up);
  glm::mat4 proj = glm::perspective(glm::radians(param.cam.FoV), 1.6666f,
                                    param.cam.near, param.cam.far);

  glm::mat4 mvp_ = proj * view * param.model2world;
  Eigen::Matrix4f mvp = Eigen::Map<Matrix4f>( glm::value_ptr(mvp_) );

  //actual drawing
  this->shader.bind();
  this->shader.setUniform("mvp", mvp);
  this->shader.setUniform("model_color", param.model_color);

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
