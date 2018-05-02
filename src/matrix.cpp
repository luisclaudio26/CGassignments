#include "../include/matrix.h"
#include <cmath>

//---------------------------------
//-------------- vec2 -------------
//---------------------------------
vec2::vec2()                  { e[0] = e[1] = 0.0f; }
vec2::vec2(float x, float y)  { e[0] = x; e[1] = y;}
float vec2::operator()(int i) const { return e[i]; }
float& vec2::operator()(int i) { return e[i]; }
vec2 vec2::operator-(const vec2& rhs) const
{
  return vec2(e[0]-rhs(0), e[1]-rhs(1));
}

//---------------------------------
//-------------- vec3 -------------
//---------------------------------
vec3::vec3() { for(int i = 0; i < 3; ++i) e[i] = 0.0f; }
vec3::vec3(float x, float y, float z) { e[0] = x; e[1] = y; e[2] = z; }
float vec3::operator()(int i) const { return e[i]; }
float& vec3::operator()(int i) { return e[i]; }
vec3 vec3::cross(const vec3& rhs) const
{
  return vec3(e[1]*rhs(2)-e[2]*rhs(1),
              e[2]*rhs(0)-e[0]*rhs(2),
              e[0]*rhs(1)-e[1]*rhs(0));
}
float vec3::dot(const vec3& rhs) const
{
  return e[0]*rhs(0)+e[1]*rhs(1)+e[2]*rhs(2);
}

vec3 vec3::unit() const
{
  float norm = (*this).dot(*this);
  return vec3(e[0]/norm, e[1]/norm, e[2]/norm);
}

vec3 vec3::operator-() const
{
  return vec3(-e[0], -e[1], -e[2]);
}
vec3 vec3::operator+(const vec3& rhs) const
{
  return vec3(e[0]+rhs(0), e[1]+rhs(1), e[2]+rhs(2));
}
vec3 vec3::operator-(const vec3& rhs) const
{
  return (*this) + (-rhs);
}
vec3 vec3::operator*(float k) const
{
  return vec3(e[0]*k, e[1]*k, e[2]*k);
}

//---------------------------------
//-------------- vec4 -------------
//---------------------------------
vec4::vec4()
{
  for(int i = 0; i < 4; ++i) e[i] = 0.0f;
}

vec4::vec4(const vec3& v, float w)
{
  for(int i = 0; i < 3; ++i)
    e[i] = v(i);
  e[3] = w;
}

vec4::vec4(float x, float y, float z, float w)
{
  e[0] = x; e[1] = y; e[2] = z; e[3] = w;
}

float vec4::operator()(int i) const
{
  return e[i];
}

float& vec4::operator()(int i)
{
  return e[i];
}

vec4 vec4::operator+(const vec4& rhs) const
{
  return vec4(e[0]+rhs(0),
              e[1]+rhs(1),
              e[2]+rhs(2),
              e[3]+rhs(3));
}

vec4 vec4::operator-() const
{
  return vec4(-e[0], -e[1], -e[2], -e[3]);
}

vec4 vec4::operator-(const vec4& rhs) const
{
  return (*this) + (-rhs);
}

float vec4::dot(const vec4& rhs) const
{
  float acc = 0.0f;
  for(int i = 0; i < 4; ++i) acc += e[i]*rhs(i);
  return acc;
}

vec4 vec4::unit() const
{
  float norm = sqrtf((*this).dot(*this));
  return vec4(e[0]/norm, e[1]/norm, e[2]/norm, e[3]/norm);
}

//---------------------------------
//-------------- mat4 -------------
//---------------------------------
mat4::mat4()
{
  for(int i = 0; i < 16; ++i)
    e[i] = 0.0f;
}

mat4::mat4(const vec4& c1, const vec4& c2, const vec4& c3, const vec4& c4)
{
  for(int i = 0; i < 4; ++i)
  {
    (*this)(i,0) = c1(i);
    (*this)(i,1) = c2(i);
    (*this)(i,2) = c3(i);
    (*this)(i,3) = c4(i);
  }
}

float& mat4::operator()(int i, int j)
{
  return e[i+4*j];
}

float mat4::operator()(int i, int j) const
{
  return e[i+4*j];
}

mat4 mat4::operator*(const mat4& rhs) const
{
  mat4 out;
  for(int i = 0; i < 4; ++i)
    for(int j = 0; j < 4; ++j)
      for(int k = 0; k < 4; ++k)
        out(i,j) += (*this)(i,k) * rhs(k,j);

  return out;
}

vec4 mat4::operator*(const vec4& rhs) const
{
  vec4 out;
  for(int i = 0; i < 4; ++i)
    for(int k = 0; k < 4; ++k)
      out(i) += (*this)(i,k) * rhs(k);
  return out;
}
