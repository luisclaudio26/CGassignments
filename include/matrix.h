#ifndef MATRIX_H
#define MATRIX_H

class vec3
{
private:
  float e[3];

public:
  vec3();
  vec3(float x, float y, float z);

  float operator()(int i) const;
  float& operator()(int i);

  vec3 operator-() const;
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

  //--------- Operators ---------
  mat4 operator*(const mat4& rhs) const;
  vec4 operator*(const vec4& rhs) const;

  //--------- Matrix constructors ---------
  static mat4 view(const vec3& eye, const vec3& look_dir, const vec3& up)
  {
    vec3 w = -look_dir.unit(); //translation is inverted in the end. check this!
    vec3 u = (w.cross(up)).unit();
    vec3 v = u.cross(w);

    return mat4(vec4(u(0), v(0), w(0), 0.0f),
                vec4(u(1), v(1), w(1), 0.0f),
                vec4(u(2), v(2), w(2), 0.0f),
                vec4(-eye.dot(u), -eye.dot(v), -eye.dot(w), 1.0f));
  }
};

#endif
