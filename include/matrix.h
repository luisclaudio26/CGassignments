#ifndef MATRIX_H
#define MATRIX_H

class Vec4
{
private:
  float e[4];

public:
  Vec4();
  Vec4(float x, float y, float z, float w);

  float operator()(int i) const;
  float& operator()(int i);

  Vec4 operator+(const Vec4& rhs) const;
  float dot(const Vec4& rhs) const;
};

class Mat4
{
private:
  float e[16];

public:
  Mat4();
  Mat4(const Vec4& c1, const Vec4& c2, const Vec4& c3, const Vec4& c4);

  float& operator()(int i, int j);
  float operator()(int i, int j) const;

  Mat4 operator*(const Mat4& rhs) const;
  Vec4 operator*(const Vec4& rhs) const;
};

#endif
