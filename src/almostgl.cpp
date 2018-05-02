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

  //we need 7 floats per vertex (4 -> XYZW, 3 -> RGB)
  //normals won't be forwarded out of vertex processing
  //stage, so we don't need to store them in the vertex
  //buffer
  vertex_sz = 4 + 3;
  n_vertices = model.mPos.cols();

  //preallocate buffers where we'll store the transformed,
  //clipped and culled vertices (triangles) before copying them to the GPU.
  //Buffers size are extremely conservative so we don't need to manage
  //memory in anyway, just allocate once and use it (no need for resizing)
  vbuffer = new float[n_vertices*vertex_sz];
  clipped = new float[n_vertices*vertex_sz];
  projected = new float[n_vertices*vertex_sz];
  culled = new float[n_vertices*vertex_sz];

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
  //TODO: we could precompute most of these calls
  vec3 eye = vec3(param.cam.eye[0], param.cam.eye[1], param.cam.eye[2]);
  vec3 up = vec3(param.cam.up[0], param.cam.up[1], param.cam.up[2]);
  vec3 right = vec3(param.cam.right[0], param.cam.right[1], param.cam.right[2]);
  vec3 look_dir = vec3(param.cam.look_dir[0],
                        param.cam.look_dir[1],
                        param.cam.look_dir[2]);
  vec3 model_color = vec3(param.model_color(0),
                          param.model_color(1),
                          param.model_color(2));
  vec4 light = vec4(param.light(0),
                    param.light(1),
                    param.light(2),
                    1.0f);

  mat4 model2world;
  for(int i = 0; i < 4; ++i)
    for(int j = 0; j < 4; ++j)
      model2world(i,j) = param.model2world[j][i];

  //-------------------------------------------------------
  //------------------ GRAPHICAL PIPELINE -----------------
  //-------------------------------------------------------
  //important matrices.
  //proj and viewport could be precomputed!
  mat4 view = mat4::view(eye, eye+look_dir, up);
  mat4 proj = mat4::perspective(param.cam.FoVy, param.cam.FoVx,
                                param.cam.near, param.cam.far);
  mat4 viewport = mat4::viewport(buffer_width, buffer_height);
  mat4 vp = proj * view;

  //vertex processing stage
  for(int v_id = 0; v_id < n_vertices; ++v_id)
  {
    Eigen::Vector3f v = model.mPos.col(v_id);
    Eigen::Vector3f n = model.mNormal.col(v_id);

    //transform vertices using model view proj.
    //notice that this is akin to what we do in
    //vertex shader.
    vec4 v_world = model2world * vec4(v(0),v(1),v(2),1.0f);
    vec4 n_world = vec4(n(0),n(1),n(2),0.0f); //TODO: use inv(trans(model2world))!
    vec4 v_out = vp * v_world;

    //compute color of this vertex using phong
    //lighting model
    vec4 v2l = (light - v_world).unit();
    float diff = std::max(0.0f, v2l.dot(-n_world));
    vec3 v_color = model_color * diff;

    //copy to vbuffer -> forward to next stage
    for(int i = 0; i < 4; ++i)
      vbuffer[vertex_sz*v_id+i] = v_out(i);
    for(int i = 0; i < 3; ++i)
      vbuffer[vertex_sz*v_id+(4+i)] = v_color(i);
  }

  //primitive "clipping"
  //Loop over vbuffer taking the vertices three by three,
  //then we test if x/y/z coordinates are greater than w. If they are,
  //then this vertex is outside the view frustum. Although the correct
  //way of handling this would be to clip the triangle, we'll just
  //discard it entirely.
  //Notice that, at this moment, we're implicitly doing some sort of
  //primitive assembly when we take vertices 3 by 3 to build a triangle
  memset(clipped, 0, sizeof(float)*n_vertices*vertex_sz);
  int clipped_last = 0;
  for(int t_id = 0; t_id < n_vertices; t_id += 3)
  {
    bool discard_tri = false;

    //loop over vertices of this triangle. if any of them
    //is outside the view frustum, discard it
    for(int v_id = 0; v_id < 3; ++v_id)
    {
      //triangle with index t_id (0, 3, 6, ...) starts at the position
      //vertex_sz * t_id in the vbuffer. each vertex v_id of t_id starts
      //at positions t_id+0, t_id+vertex_sz, t_id+2vertex_sz.
      //XYZW in v_id are in +0, +1, +2, +3, RGB in +4,+5,+6
      int v = vertex_sz*t_id + vertex_sz*v_id;
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
      //Here we learn that it is always better name the constants vertex_sz
      //and blablabla instead of just throwing 12's, 7's, 4's around =)
      memcpy(&clipped[clipped_last], &vbuffer[vertex_sz*t_id], 3*vertex_sz*sizeof(float));
      clipped_last += 3*vertex_sz;
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
     #define ROUND(x) ((int)(x + 0.5f))
     #define FLOOR(x) ((int)x)
     #define CEIL(x)  ((int)(x+0.9999f))
     #define EQ(x,y)  (std::fabs(x-y) < 0.001f)

     //apply viewport transformation to each vertex
     vec4 v0_ = viewport*vec4(culled[p_id+0], culled[p_id+1], 1.0f, 1.0f);
     vec4 v1_ = viewport*vec4(culled[p_id+2], culled[p_id+3], 1.0f, 1.0f);
     vec4 v2_ = viewport*vec4(culled[p_id+4], culled[p_id+5], 1.0f, 1.0f);

     vec2 v0(ROUND(v0_(0)), ROUND(v0_(1)));
     vec2 v1(ROUND(v1_(0)), ROUND(v1_(1)));
     vec2 v2(ROUND(v2_(0)), ROUND(v2_(1)));

     //order triangles by y coordinate
     #define SWAP(a,b) { vec2 aux = b; b = a; a = aux; }
     if( v0(1) > v1(1) ) SWAP(v0, v1);
     if( v0(1) > v2(1) ) SWAP(v0, v2);
     if( v1(1) > v2(1) ) SWAP(v1, v2);

     //loop from v0 to the vertex in the middle,
     //drawing the first "half" of the triangle
     float e0 = (v1(0)-v0(0))/(v1(1)-v0(1));
     float e1 = (v2(0)-v0(0))/(v2(1)-v0(1));
     float e2 = (v2(0)-v1(0))/(v2(1)-v1(1));
     float start, end;
     float start_dx, end_dx;

     //TODO: add edges for z position and vertex color

     //TODO: transform this into a boolean so we can use
     //the same next_active in all edges (we'll need edges
     //for z position and vertex color also)
     float *next_active;

     //decide start/end edges. If v1 is to the left
     //side of the edge connecting v0 and v2, then v0v1
     //is the starting edge and v0v2 is the ending edge;
     //if v1 is to the right, it is the contrary.
     //the v0v1 edge will be substituted by the v1v2 edge
     //when we reach the v1 vertex while scanlining, so we
     //store which of the start/end edges we should replace
     //with v1v2.
     vec3 right_side = vec3(v1(0)-v0(0), v1(1)-v0(1), 0.0f).cross(vec3(v2(0)-v0(0), v2(1)-v0(1), 0.0f));
     if( right_side(2) > 0 )
     {
       end_dx = e0;
       start_dx = e1;
       next_active = &end_dx;
     }
     else
     {
       end_dx = e1;
       start_dx = e0;
       next_active = &start_dx;
     }

     //handle flat top triangles
     if( v0(1) == v1(1) )
     {
       //switch active edge and update
       //starting and ending points
       if( v0(0) < v1(0) )
       {
         end_dx = e2;
         start = v0(0); end = v1(0);
       }
       else
       {
         start_dx = e2;
         start = v1(0); end = v0(0);
       }
     }
     else start = end = v0(0);

     //loop over scanlines
     for(int y = v0(1); y <= v2(1); ++y)
     {
       //rasterize scanline
       for(int x = ROUND(start); x <= ROUND(end); ++x)
         SET_PIXEL(y, x, 255, 255, 255);

       //switch active edges if halfway through the triangle
       //This MUST be done before incrementing, otherwise
       //once we reached v1 we would pass through it and
       //start coming back only in the next step, causing
       //the big "leaking" triangles!
       if( y == (int)v1(1) ) *next_active = e2;

       //increment bounds
       start += start_dx; end += end_dx;
     }
   }

  //-------------------------------------------------------
  //---------------------- DISPLAY ------------------------
  //-------------------------------------------------------
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

  this->shader.bind();
  this->shader.setUniform("frame", 0);

  //draw stuff
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  this->shader.drawArray(GL_TRIANGLES, 0, 6);

  //compute time
  t = clock() - t;
  this->framerate = CLOCKS_PER_SEC/(float)t;
}
