#version 450

// from vertex shader
in vec2 uv_frag;

// fragment final color
out vec4 color;

// the actual color buffer
uniform sampler2D frame;

void main()
{
  color = texture(frame, uv_frag);
}
