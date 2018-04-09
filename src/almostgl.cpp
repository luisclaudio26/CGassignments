#include "../include/almostgl.h"


AlmostGL::AlmostGL(const char* path, Widget *parent) : nanogui::GLCanvas(parent)
{
  this->model.load_file(path);
  this->model.transform_to_center(mModel);

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

  glm::vec3 mEye = glm::vec3(0.0f, 0.0f, 0.0f);
  glm::vec3 mLookDir = glm::vec3(0.0f, 0.0f, -1.0f);
  glm::vec3 mUp = glm::vec3(0.0f, 1.0f, 0.0f);
  glm::vec3 mRight = glm::cross(mLookDir, mUp);
  float mNear = 1.0f, mFar = 10.0f;
  float mStep = 0.1f;
  bool mLockView = false;
  float mFoV = 45.0;
  Eigen::Vector4f mModelColor; mModelColor<<0.0f,1.0f,0.0f,1.0f;

  //uniform uploading
  glm::mat4 view = glm::lookAt(mEye, mEye + mLookDir, mUp);
  glm::mat4 proj = glm::perspective(glm::radians(mFoV), 1.6666f, mNear, mFar);

  glm::mat4 mvp_ = proj * view * mModel;
  Eigen::Matrix4f mvp = Eigen::Map<Matrix4f>( glm::value_ptr(mvp_) );

  //actual drawing
  this->shader.bind();
  this->shader.setUniform("mvp", mvp);
  this->shader.setUniform("model_color", mModelColor);

  //Z buffering
  glEnable(GL_DEPTH_TEST);

  //Backface/Frontface culling
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);

  //draw mode
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  this->shader.drawArray(GL_TRIANGLES, 0, this->model.mPos.cols());

  //disable options
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glDisable(GL_DEPTH_TEST);
}
