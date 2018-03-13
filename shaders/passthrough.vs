#version 450

// from host
in vec3 pos;
in vec3 color;

// to fragment shader
out vec3 vcolor;

void main()
{
  gl_Position.xyz = pos;
  gl_Position.w = 1.0f;

  vcolor = color;
}
