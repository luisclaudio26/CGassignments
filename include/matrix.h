#ifndef MATRIX_H
#define MATRIX_H

#include <cmath>

#define PI 3.14159265f

class vec3
{
private:
  float e[3];

public:
  vec3();
  vec3(float x, float y, float z);

  float operator()(int i) const;
  float& operator()(int i);

  vec3 operator+(const vec3& rhs) const;
  vec3 operator-() const;
  vec3 operator-(const vec3& rhs) const;
  vec3 cross(const vec3& rhs) const;
  float dot(const vec3& rhs) const;
  vec3 unit() const;
};

class vec4
{
private:
  float e[4];

public:
  vec4();
  vec4(float x, float y, float z, float w);

  float operator()(int i) const;
  float& operator()(int i);

  vec4 operator+(const vec4& rhs) const;
  vec4 operator-() const;
  float dot(const vec4& rhs) const;
  vec4 cross(const vec4& rhs) const;
  vec4 unit() const;
};

class mat4
{
private:
  float e[16];

public:
  mat4();
  mat4(const vec4& c1, const vec4& c2, const vec4& c3, const vec4& c4);

  float& operator()(int i, int j);
  float operator()(int i, int j) const;
  float* data() { return e; }

  //--------- Operators ---------
  mat4 operator*(const mat4& rhs) const;
  vec4 operator*(const vec4& rhs) const;

  //--------- Matrix constructors ---------
  static mat4 view(const vec3& eye, const vec3& look_at, const vec3& up)
  {
    vec3 w = (eye-look_at).unit(); //translation is inverted in the end. check this!
    vec3 u = (up.cross(w)).unit();
    vec3 v = w.cross(u);

    return mat4(vec4(u(0), v(0), w(0), 0.0f),
                vec4(u(1), v(1), w(1), 0.0f),
                vec4(u(2), v(2), w(2), 0.0f),
                vec4(-eye.dot(u), -eye.dot(v), -eye.dot(w), 1.0f));
  }

  static mat4 perspective(float fov, float aspect_ratio, float near, float far)
  {
      //compute bounds of the projection plane
      float fov_ = fov * 0.5f;

      float t = tan(fov_ * PI / 180.0f) * near; float b = -t;
      float r = t * aspect_ratio; float l = -r;

      //shearing matrix. TODO: we don't really need this in this version,
      //only for skewed frustums
      float g = (l+r)/(2*near), h = (t+b)/(2*near);
      mat4 shear = mat4( vec4(1.0f, 0.0f, 0.0f, 0.0f),
                          vec4(0.0f, 1.0f, 0.0f, 0.0f),
                          vec4(g, h, 1.0f, 0.0f),
                          vec4(0.0f, 0.0f, 0.0f, 1.0f));

      //scaling matrix. Maps frustum to another frustum with fov = 90 deg
      //by computing a scaling that makes the corners of the viewplane to
      //be in (near,near), (near,-near), (-near,-near), (-near,near)
      float sx = (2*near)/(r-l), sy = (2*near)/(t-b);
      mat4 scale = mat4( vec4(sx, 0.0f, 0.0f, 0.0f),
                          vec4(0.0f, sy, 0.0f, 0.0f),
                          vec4(0.0f, 0.0f, 1.0f, 0.0f),
                          vec4(0.0f, 0.0f, 0.0f, 1.0f) );

      //normalizing transform. assuming that the viewplane is at d = -1,
      //and knowing that to project a point at (x,y,z) in this plane we just
      //need to divide by -z (i.e., the projected point is mapped to (-x/z, -y/z, -1)),
      //this matrix, when multiplied by a vector, makes w = -z so when dividing
      //the point is projected. Also, it scales and translates the view frustum
      //so that the mapped points (after division by w) are within the cube
      //of corners (-1,-1,-1) (1,1,1)
      float alpha = (near+far)/(near-far), beta = (2*near*far)/(near-far);
      //float alpha = far/(near-far), beta = (near*far)/(near-far);
      mat4 norm = mat4( vec4(1.0f, 0.0f, 0.0f, 0.0f),
                        vec4(0.0f, 1.0f, 0.0f, 0.0f),
                        vec4(0.0f, 0.0f, alpha, -1.0f),
                        vec4(0.0f, 0.0f, beta, 0.0f) );

      //final matrix is just the three above composed
      return norm * scale * shear;
  }

};

#endif
