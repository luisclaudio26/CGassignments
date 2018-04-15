#version 450

// from host
in vec2 pos;
in vec3 normal;
in vec3 amb, diff, spec;
in float shininess;

// to fragment shader
out vec3 f_amb, f_diff, f_spec;
out float f_shininess;
out vec3 f_normal;

// uniform variables
//uniform mat4 mvp;

void main()
{
  gl_Position.xy = pos;
  gl_Position.z = 1.0f;
  gl_Position.w = 1.0f;

  //propagate variables
  //f_normal = (inverse(transpose(mvp))*vec4(normal, 0.0)).xyz;
  f_normal = normal;
  f_amb = amb; f_diff = diff; f_spec = spec;
  f_shininess = shininess;
}
