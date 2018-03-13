#version 450

// from fragment shader
in vec3 vcolor;

// fragment final color
out vec3 fcolor;

void main()
{
  fcolor = vcolor;
}
