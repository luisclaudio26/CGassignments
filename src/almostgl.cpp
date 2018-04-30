#include "../include/almostgl.h"
#include "../include/matrix.h"
#include <glm/gtc/matrix_access.hpp>
#include <nanogui/opengl.h>
#include <cstring>
#include <ctime>

AlmostGL::AlmostGL(const GlobalParameters& param,
                    const char* path,
                    Widget *parent) : nanogui::GLCanvas(parent), param(param)
{
  this->model.load_file(path);

  //We'll use the same model matrix for both contexts, as we
  //are loading the same model on both.
  //this->model.transform_to_center(mModel);

  this->shader.initFromFiles("almostgl",
                              "../shaders/almostgl.vs",
                              "../shaders/almostgl.fs");

  //this will serve us both as screen coordinates for the quad
  //AND texture coordinates
  Eigen::MatrixXf quad(2, 6);
  quad.col(0)<<-1.0, -1.0;
  quad.col(1)<<+1.0, -1.0;
  quad.col(2)<<+1.0, +1.0;
  quad.col(3)<<-1.0, -1.0;
  quad.col(4)<<+1.0, +1.0;
  quad.col(5)<<-1.0, +1.0;

  //for some reason, OpenGL inverts the v axis,
  //so we undo this here
  Eigen::MatrixXf texcoord(2, 6);
  texcoord.col(0)<<0.0f, 1.0f;
  texcoord.col(1)<<1.0f, 1.0f;
  texcoord.col(2)<<1.0f, 0.0f;
  texcoord.col(3)<<0.0f, 1.0f;
  texcoord.col(4)<<1.0f, 0.0f;
  texcoord.col(5)<<0.0f, 0.0f;

  this->shader.bind();
  this->shader.uploadAttrib<Eigen::MatrixXf>("quad_pos", quad);
  this->shader.uploadAttrib<Eigen::MatrixXf>("quad_uv", texcoord);

  //preallocate buffers where we'll store the transformed,
  //clipped and culled vertices (triangles) before copying them to the GPU
  vbuffer = new float[model.mPos.cols()*4];
  clipped = new float[model.mPos.cols()*4];
  projected = new float[model.mPos.cols()*2];
  culled = new float[model.mPos.cols()*2];

  //preallocate color and depth buffers with the
  //initial window size. this will once we resize the window!
  glGenTextures(1, &color_gpu);

  buffer_height = this->height(); buffer_width = this->width();
  int n_pixels = buffer_width * buffer_height;

  color = new GLubyte[4*n_pixels];
  for(int i = 0; i < n_pixels*4; i += 4) color[i] = 0;
  for(int i = 1; i < n_pixels*4; i += 4) color[i] = 0;
  for(int i = 2; i < n_pixels*4; i += 4) color[i] = 30;
  for(int i = 3; i < n_pixels*4; i += 4) color[i] = 255;

  depth = new float[n_pixels];
}

void AlmostGL::drawGL()
{
  using namespace nanogui;
  clock_t t = clock();

  //convert params to use internal library
  vec3 eye = vec3(param.cam.eye[0], param.cam.eye[1], param.cam.eye[2]);
  vec3 up = vec3(param.cam.up[0], param.cam.up[1], param.cam.up[2]);
  vec3 right = vec3(param.cam.right[0], param.cam.right[1], param.cam.right[2]);
  vec3 look_dir = vec3(param.cam.look_dir[0],
                        param.cam.look_dir[1],
                        param.cam.look_dir[2]);

  mat4 model2world;
  for(int i = 0; i < 4; ++i)
    for(int j = 0; j < 4; ++j)
      model2world(i,j) = param.model2world[j][i];

  //important matrices.
  //proj and viewport could be precomputed!
  mat4 view = mat4::view(eye, eye+look_dir, up);
  mat4 proj = mat4::perspective(param.cam.FoVy, param.cam.FoVx,
                                param.cam.near, param.cam.far);
  mat4 mvp = proj * view * model2world;
  mat4 viewport = mat4::viewport(buffer_width, buffer_height);

  //geometric transformations
  for(int v_id = 0; v_id < model.mPos.cols(); ++v_id)
  {
    Eigen::Vector3f v = model.mPos.col(v_id);
    vec4 v_ = mvp * vec4(v(0), v(1), v(2), 1.0f);

    //copy to vbuffer
    for(int i = 0; i < 4; ++i)
      vbuffer[4*v_id+i] = v_(i);
  }

  //primitive "clipping"
  //Loop over vbuffer taking the vertices three by three,
  //then we test if x/y/z coordinates are greater than w. If they are,
  //then this vertex is outside the view frustum. Although the correct
  //way of handling this would be to clip the triangle, we'll just
  //discard it entirely.
  memset(clipped, 0, sizeof(float)*model.mPos.cols()*4);
  int clipped_last = 0;
  for(int t_id = 0; t_id < model.mPos.cols(); t_id += 3)
  {
    bool discard_tri = false;

    //loop over vertices of this triangle. if any of them
    //is outside the view frustum, discard it
    for(int v_id = 0; v_id < 3; ++v_id)
    {
      int v = 4*t_id + 4*v_id;
      float w = vbuffer[v+3];

      //near plane clipping
      if( w <= 0 )
      {
        discard_tri = true;
        break;
      }

      //clip primitives outside frustum
      if(std::fabs(vbuffer[v+0]) > w ||
          std::fabs(vbuffer[v+1]) > w ||
          std::fabs(vbuffer[v+2]) > w)
      {
        discard_tri = true;
        break;
      }
    }

    if(!discard_tri)
    {
      memcpy(&clipped[clipped_last], &vbuffer[4*t_id], 12*sizeof(float));
      clipped_last += 12;
    }
  }

  //perspective division
  int projected_last = 0;
  for(int v_id = 0; v_id < clipped_last; v_id += 4)
  {
    float &x = projected[projected_last+0];
    float &y = projected[projected_last+1];

    float w = clipped[v_id+3];
    x = clipped[v_id+0]/w;
    y = clipped[v_id+1]/w;

    projected_last += 2;
  }

  //triangle culling
  int culled_last = 0;
  for(int v_id = 0; v_id < projected_last; v_id += 6)
  {
    vec3 v0(projected[v_id+0], projected[v_id+1], 1.0f);
    vec3 v1(projected[v_id+2], projected[v_id+3], 1.0f);
    vec3 v2(projected[v_id+4], projected[v_id+5], 1.0f);

    //compute cross product p = v0v1 X v0v2;
    //if p is pointing outside the screen, v0v1v2 are defined
    //in counter-clockwise order. then, reject or accept this
    //triangle based on the param.front_face flag.
    vec3 c = (v1-v0).cross(v2-v0);

    //cull clockwise triangles
    if(param.front_face == GL_CCW && c(2) < 0 ||
        param.front_face == GL_CW && c(2) > 0) continue;

    //copy to final buffer
    memcpy(&culled[culled_last], &projected[v_id], 6*sizeof(float));
    culled_last += 6;
  }

  //rasterization
  #define PIXEL(i,j) (4*(i*buffer_width+j))
  #define SET_PIXEL(i,j,r,g,b) { color[PIXEL(i,j)+0] = r; \
                                 color[PIXEL(i,j)+1] = g; \
                                 color[PIXEL(i,j)+2] = b; \
                                 color[PIXEL(i,j)+3] = 255;}

   //clear color buffer
   memset((void*)color, 0, (4*buffer_width*buffer_height)*sizeof(GLubyte));

   for(int p_id = 0; p_id < culled_last; p_id += 6)
   {
     //apply viewport transformation to each vertex
     vec4 v0_ = viewport*vec4(culled[p_id+0], culled[p_id+1], 1.0f, 1.0f);
     vec4 v1_ = viewport*vec4(culled[p_id+2], culled[p_id+3], 1.0f, 1.0f);
     vec4 v2_ = viewport*vec4(culled[p_id+4], culled[p_id+5], 1.0f, 1.0f);

     vec2 v0(v0_(0), v0_(1));
     vec2 v1(v1_(0), v1_(1));
     vec2 v2(v2_(0), v2_(1));

     //order triangles by y coordinate
     #define SWAP(a,b) { vec2 aux = b; b = a; a = aux; }
     if( v0(1) > v1(1) ) SWAP(v0, v1);
     if( v0(1) > v2(1) ) SWAP(v0, v2);
     if( v1(1) > v2(1) ) SWAP(v1, v2);

     #define ROUND(x) ((int)(x + 0.5f))
     //loop from v0 to the vertex in the middle,
     //drawing the first "half" of the triangle
     float start = v0(0), end = v0(0);

     float e0dx = (v1(0)-v0(0))/(v1(1)-v0(1));
     float e1dx = (v2(0)-v0(0))/(v2(1)-v0(1));
     float start_dx, end_dx;

     //TODO: find a more compact expression for this
     /*
     if(v1(0) == v2(0))
     {
       if(v1(0) < v0(0))
       {
         start_dx = e0dx;
         end_dx = e1dx;
       }
       else
       {
         start_dx = e1dx;
         end_dx = e0dx;
       }
     }
     else if(v1(0) < v2(0))
     {
       start_dx = e0dx;
       end_dx = e1dx;
     }
     else if(v1(0) > v2(0))
     {
       start_dx = e1dx;
       end_dx = e0dx;
     } */
     if(v1(0) < v2(0))
     {
       start_dx = e0dx;
       end_dx = e1dx;
     }
     else if(v1(0) > v2(0))
     {
       start_dx = e1dx;
       end_dx = e0dx;
     }

     /*
     for(int y = ROUND(v0(1)); y < ROUND(v1(1)); ++y)
     {
       for(int x = ROUND(start); x < ROUND(end); ++x)
         SET_PIXEL(y, x, 255, 255, 255);
       start += start_dx; end += end_dx;
     }

     //loop over second half
     float e2dx = (v2(0)-v1(0))/(v2(1)-v1(1));
     if(v1(0) < v2(0)) start_dx = e2dx;
     else end_dx = e2dx;

     for(int y = ROUND(v1(1)); y < ROUND(v2(1)); ++y)
     {
       for(int x = ROUND(start); x < ROUND(end); ++x)
         SET_PIXEL(y, x, 255, 255, 255);
       start += start_dx; end += end_dx;
     } */

     for(int y = ROUND(v0(1)); y < ROUND(v2(1)); ++y)
     {
       //rasterize scanline
       for(int x = ROUND(start); x < ROUND(end); ++x)
         SET_PIXEL(y, x, 255, 255, 255);

       //switch active edge if we reached halfway the triangle
       if( y == ROUND(v1(1)) )
       {
         float dy_v2v1 = v2(1)-v1(1);

         //just ignore the last scanline if edge is horizontal
         if(dy_v2v1 == 0.0f) break;

         float e2dx = (v2(0)-v1(0))/dy_v2v1;
         if(v1(0) < v2(0)) start_dx = e2dx;
         else end_dx = e2dx;
       }

       //compute increments for the next scanline bounds
       start += start_dx; end += end_dx;
     }

   }

  // send to GPU in texture unit 0
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, color_gpu);

  //WARNING: be careful with RGB pixel data as OpenGL
  //expects 4-byte aligned data
  //https://www.khronos.org/opengl/wiki/Common_Mistakes#Texture_upload_and_pixel_reads
  glPixelStorei(GL_UNPACK_LSB_FIRST, 0);
  glTexImage2D(GL_TEXTURE_2D,
                0,
                GL_RGBA8,
                buffer_width,
                buffer_height,
                0,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                color);

  //WARNING: IF WE DON'T SET THIS IT WON'T WORK!
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  //-- we unfortunately need to use uploadAttrib which will call glBufferData
  //-- under the hood. Using glBufferSubData() won't work.
  this->shader.bind();
  this->shader.setUniform("frame", 0);

  //draw stuff
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  this->shader.drawArray(GL_TRIANGLES, 0, 6);

  //compute time
  t = clock() - t;
  this->framerate = CLOCKS_PER_SEC/(float)t;
}
