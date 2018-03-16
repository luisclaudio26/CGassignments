#version 450

// from host
in vec3 pos, normal;
in vec3 amb, diff, spec;
in float shininess;

// to fragment shader
out vec3 f_amb, f_diff, f_spec;
out float f_shininess;
out vec3 f_normal;

// uniform variables
uniform mat4 mvp;

void main()
{
  gl_Position = mvp * vec4(pos, 1.0f);

  //propagate variables
  f_normal = (inverse(transpose(mvp))*vec4(normal, 0.0f)).xyz;
  f_amb = amb; f_diff = diff; f_spec = spec;
  f_shininess = shininess;
}
