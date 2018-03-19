#version 450

// from vertex shader
in vec3 f_amb, f_diff, f_spec;
in float f_shininess;
in vec3 f_normal;

// fragment final color
out vec3 color;

void main()
{
  //non sense operation. we need this so the compiler won't erase
  //the variables, causing problems with attribute uploading
  //color = f_shininess*(f_amb + f_diff + f_spec + f_normal);

  color = vec3(0.0, 0.6, 0.0);
}
