#include "../include/ogl.h"

OGL::OGL(GlobalParameters& param,
          const char* path,
          Widget *parent) : nanogui::GLCanvas(parent), param(param), framerate(0.0f)
{
  this->model.load_file(path);

  //We'll use the same model matrix for both contexts, as we
  //are loading the same model on both.
  //this->model.transform_to_center(mModel);

  this->shader.initFromFiles("phong",
                              "../shaders/phong.vs",
                              "../shaders/phong.fs");

  this->shader.bind();
  this->shader.uploadAttrib<Eigen::MatrixXf>("pos", model.mPos);
  this->shader.uploadAttrib<Eigen::MatrixXf>("normal", model.mNormal);
  this->shader.uploadAttrib<Eigen::MatrixXf>("amb", model.mAmb);
  this->shader.uploadAttrib<Eigen::MatrixXf>("diff", model.mDiff);
  this->shader.uploadAttrib<Eigen::MatrixXf>("spec", model.mSpec);
  this->shader.uploadAttrib<Eigen::MatrixXf>("shininess", model.mShininess);
}

void OGL::drawGL()
{
  using namespace nanogui;
  clock_t delta = clock();

  //uniform uploading
  glm::mat4 view = glm::lookAt(param.cam.eye,
                                param.cam.eye + param.cam.look_dir,
                                param.cam.up);

  float t = tan( glm::radians(param.cam.FoVy/2) ), b = -t;
  float r = tan( glm::radians(param.cam.FoVx/2) ), l = -r;
  glm::mat4 proj = glm::frustum(l, r, b, t, param.cam.near, param.cam.far);

  Eigen::Matrix4f m = Eigen::Map<Eigen::Matrix4f>(glm::value_ptr(param.model2world));
  Eigen::Vector3f eye = Eigen::Map<Eigen::Vector3f>(glm::value_ptr(param.cam.eye));

  Eigen::Matrix4f v = Eigen::Map<Eigen::Matrix4f>(glm::value_ptr(view));
  Eigen::Matrix4f p = Eigen::Map<Eigen::Matrix4f>(glm::value_ptr(proj));

  //actual drawing
  this->shader.bind();
  this->shader.setUniform("model", m);
  this->shader.setUniform("view", v);
  this->shader.setUniform("proj", p);
  this->shader.setUniform("eye", eye);
  this->shader.setUniform("light", param.light);
  this->shader.setUniform("model_color", param.model_color);
  this->shader.setUniform("shadeId", param.shading);

  //Z buffering
  glEnable(GL_DEPTH_TEST);

  //Backface/Frontface culling
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(param.front_face);

  //draw mode
  glPolygonMode(GL_FRONT_AND_BACK, param.draw_mode);

  this->shader.drawArray(GL_TRIANGLES, 0, model.mPos.cols());

  //disable options
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glDisable(GL_DEPTH_TEST);

  //compute time
  delta = clock() - delta;
  this->framerate = CLOCKS_PER_SEC/(float)delta;
}
