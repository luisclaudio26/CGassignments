#include "../include/matrix.h"

//---------------------------------
//-------------- Vec4 -------------
//---------------------------------
Vec4::Vec4()
{
  for(int i = 0; i < 4; ++i) e[i] = 0.0f;
}

Vec4::Vec4(float x, float y, float z, float w)
{
  e[0] = x; e[1] = y; e[2] = z; e[3] = w;
}

float Vec4::operator()(int i) const
{
  return e[i];
}

float& Vec4::operator()(int i)
{
  return e[i];
}

Vec4 Vec4::operator+(const Vec4& rhs) const
{
  return Vec4(e[0]+rhs(0),
              e[1]+rhs(1),
              e[2]+rhs(2),
              e[3]+rhs(3));
}

float Vec4::dot(const Vec4& rhs) const
{
  float acc = 0.0f;
  for(int i = 0; i < 4; ++i) acc += e[i]*rhs(i);
  return acc;
}

//---------------------------------
//-------------- Mat4 -------------
//---------------------------------
Mat4::Mat4()
{
  for(int i = 0; i < 16; ++i)
    e[i] = 0.0f;
}

Mat4::Mat4(const Vec4& c1, const Vec4& c2, const Vec4& c3, const Vec4& c4)
{
  for(int i = 0; i < 4; ++i)
  {
    (*this)(i,0) = c1(i);
    (*this)(i,1) = c2(i);
    (*this)(i,2) = c3(i);
    (*this)(i,3) = c4(i);
  }
}

float& Mat4::operator()(int i, int j)
{
  return e[i+4*j];
}

float Mat4::operator()(int i, int j) const
{
  return e[i+4*j];
}

Mat4 Mat4::operator*(const Mat4& rhs) const
{
  Mat4 out;
  for(int i = 0; i < 4; ++i)
    for(int j = 0; j < 4; ++j)
      for(int k = 0; k < 4; ++k)
        out(i,j) += (*this)(i,k) * rhs(k,j);

  return out;
}

Vec4 Mat4::operator*(const Vec4& rhs) const
{
  Vec4 out;
  for(int i = 0; i < 4; ++i)
    for(int k = 0; k < 4; ++k)
      out(i) += (*this)(i,k) * rhs(k);
  return out;
}
