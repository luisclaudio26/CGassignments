#version 450

// from vertex shader
in vec3 f_amb, f_diff, f_spec;
in float f_shininess;
in vec3 f_normal;

// fragment final color
out vec4 color;

// uniforms
uniform vec4 model_color; //this sets a unique color for
                          //all triangles, as requested in
                          //the assignment

void main()
{
  //non sense operation. we need this so the compiler won't erase
  //the variables, causing problems with attribute uploading
  //color = f_shininess*(f_amb + f_diff + f_spec + f_normal);

  color = model_color;
}
