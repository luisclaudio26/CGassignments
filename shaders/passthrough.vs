#version 450

// from host
in vec3 pos, normal;
in vec3 amb, diff, spec;
in float shininess;

// to fragment shader
out vec3 f_amb, f_diff, f_spec;
out float f_shininess;
out vec3 f_normal;

// the sacred matrices
uniform mat4 model, view, proj;

// scene settings
uniform vec3 eye;

//light settings
vec3 l = vec3(0.0f);

void main()
{
  mat4 mvp = proj * view * model;
  gl_Position = mvp * vec4(pos, 1.0);

  //Phong illumination model with Gouraud shading
  //Everything occurs in world space
  vec3 pos_worldspace = (model * vec4(pos, 1.0f)).xyz;
  vec3 v2l = normalize(l - pos_worldspace);
  vec3 v2e = normalize(eye - pos_worldspace);
  vec3 h = normalize(v2l+v2e);

  float diff_k = max(0.0f, dot(-normal, v2l));
  float spec_k = max(0.0f, pow(dot(h, -normal) ,15.0f));


  f_amb = amb;
  f_diff = diff * diff_k;
  f_spec = spec * spec_k;
  f_shininess = shininess;

  //propagate variables
  f_normal = (inverse(transpose(mvp))*vec4(normal, 0.0)).xyz;
}
