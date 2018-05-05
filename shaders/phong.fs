#version 450

// from vertex shader
in vec3 lerp_amb, lerp_diff, lerp_spec;
in float lerp_shininess;
in vec3 lerp_normal, lerp_eye, lerp_pos;

// fragment final color
out vec4 color_out;

// scene settings
uniform vec3 eye, light;
uniform vec3 model_color;

// illumination models
uniform int shadeId;

vec3 gouraudAD()
{
  return lerp_amb + lerp_diff;
}

vec3 gouraudADS()
{
  return lerp_amb + lerp_diff + lerp_spec;
}

vec3 phong()
{
  //renormalize normal. When the rasterizer
  //interpolates the attributes, the linear interpolation
  //of the normals will make their norm less than 1.
  vec3 normal = normalize(lerp_normal);
  vec3 v2l = normalize(light - lerp_pos);
  vec3 v2e = normalize(eye - lerp_pos);
  vec3 h = normalize(v2l+v2e);

  float diff_k = max(0.0f, dot(normal, v2l));
  float spec_k = max(0.0f, pow(dot(normal, h), lerp_shininess));

  return lerp_amb + model_color * diff_k + vec3(1.0f) * spec_k;
}

vec3 no_shade()
{
  return model_color;
}

void main()
{
  vec3 color;
  switch(shadeId)
  {
    case 0: color = gouraudAD(); break;
    case 1: color = gouraudADS(); break;
    case 2: color = phong(); break;
    case 3: color = no_shade(); break;
  }

  color_out = vec4(color, 1.0f);
}
