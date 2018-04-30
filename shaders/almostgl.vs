#version 450

// from host
in vec2 quad_pos;
in vec2 quad_uv;

// to fragment shader
out vec2 uv_frag;

void main()
{
  gl_Position.xy = quad_pos;
  gl_Position.zw = vec2(0.0f, 1.0f);

  uv_frag = quad_uv;
}
