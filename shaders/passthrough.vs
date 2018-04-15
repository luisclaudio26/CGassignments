#version 450

// from host
in vec3 pos, normal;
in vec3 amb, diff, spec;
in float shininess;

// to fragment shader: linear interpolated (lerp) data
out vec3 lerp_amb, lerp_diff, lerp_spec;
out float lerp_shininess;
out vec3 lerp_normal, lerp_pos;

// the sacred matrices
uniform mat4 model, view, proj;

// scene settings
uniform vec3 eye, light;
uniform vec3 model_color;

void main()
{
  mat4 mvp = proj * view * model;
  gl_Position = mvp * vec4(pos, 1.0);

  //Blinn-Phong illumination model with Gouraud shading
  //Everything occurs in world space.
  vec3 pos_worldspace = (model * vec4(pos, 1.0f)).xyz;
  vec3 v2l = normalize(light - pos_worldspace);
  vec3 v2e = normalize(eye - pos_worldspace);
  vec3 h = normalize(v2l+v2e);

  float diff_k = max(0.0f, dot(-normal, v2l));
  float spec_k = max(0.0f, pow(dot(h, -normal), shininess));

  //output to fragment shader
  lerp_amb = model_color * 0.2f;
  lerp_diff = model_color * diff_k;
  lerp_spec = vec3(1.0f) * spec_k;
  lerp_shininess = shininess;
  lerp_pos = pos_worldspace;
  lerp_normal = (inverse(transpose(model))*vec4(-normal, 0.0)).xyz;
}
