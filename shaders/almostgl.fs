#version 450

// from vertex shader
in vec2 uv_frag;

// fragment final color
out vec4 color;

// the actual color buffer
uniform sampler2D frame;

void main()
{
  color.xyz = texture(frame, uv_frag).xyz;
  color.w = 1.0f;
}
