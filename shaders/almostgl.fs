#version 450

// from vertex shader
in vec3 f_amb, f_diff, f_spec;
in float f_shininess;
in vec3 f_normal;

in vec2 uv_frag;

// fragment final color
out vec4 color;

// uniforms
uniform sampler2D frame;
uniform vec3 model_color; //this sets a unique color for
                          //all triangles, as requested in
                          //the assignment

void main()
{
  color = texture(frame, uv_frag);
}
