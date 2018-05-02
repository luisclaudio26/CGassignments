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
    //vec3 v_color = model_color * diff;
    vec3 v_color = model_color;

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
  for(int p_id = 0; p_id < n_vertices*vertex_sz; p_id += 3*vertex_sz)
  {
    bool discard_tri = false;

    //loop over vertices of this triangle. if any of them
    //is outside the view frustum, discard it
    for(int v_id = 0; v_id < 3; ++v_id)
    {
      //triangle with index p_id (0, 3, 6, ...) starts at the position
      //vertex_sz * p_id in the vbuffer. each vertex v_id of p_id starts
      //at positions p_id+0, p_id+vertex_sz, p_id+2vertex_sz.
      //XYZW in v_id are in +0, +1, +2, +3, RGB in +4,+5,+6
      int v = p_id + vertex_sz*v_id;
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
      memcpy(&clipped[clipped_last], &vbuffer[p_id], 3*vertex_sz*sizeof(float));
      clipped_last += 3*vertex_sz;
    }
  }

  //perspective division
  int projected_last = 0;
  for(int v_id = 0; v_id < clipped_last; v_id += vertex_sz)
  {
    float w = clipped[v_id+3];
    projected[projected_last+0] = clipped[v_id+0]/w;
    projected[projected_last+1] = clipped[v_id+1]/w;
    projected[projected_last+2] = clipped[v_id+2]/w;
    projected[projected_last+3] = 1.0f;
    projected[projected_last+4] = clipped[v_id+4];
    projected[projected_last+5] = clipped[v_id+5];
    projected[projected_last+6] = clipped[v_id+6];

    //we'll still keep all the 7 floats for simplicity!
    projected_last += vertex_sz;
  }

  //triangle culling
  int culled_last = 0;
  for(int p_id = 0; p_id < projected_last; p_id += 3*vertex_sz)
  {
    //Data layout per vertex inside _projected_ is:
    //... X1 Y1 Z1 W1 R1 G1 B1 X2 Y2 Z2 W2 R2 G2 B2 X3 Y3 Z3 W3 R3 G3 B3 ...
    //
    //TODO: we could guarantee optimization by using an incrementer instead of
    //computing products vertex_sz*i, but maybe the compiler already does this
    vec3 v0(projected[p_id+vertex_sz*0+0], projected[p_id+vertex_sz*0+1], 1.0f);
    vec3 v1(projected[p_id+vertex_sz*1+0], projected[p_id+vertex_sz*1+1], 1.0f);
    vec3 v2(projected[p_id+vertex_sz*2+0], projected[p_id+vertex_sz*2+1], 1.0f);

    //compute cross product p = v0v1 X v0v2;
    //if p is pointing outside the screen, v0v1v2 are defined
    //in counter-clockwise order. then, reject or accept this
    //triangle based on the param.front_face flag.
    vec3 c = (v1-v0).cross(v2-v0);

    //cull clockwise triangles
    if(param.front_face == GL_CCW && c(2) < 0 ||
        param.front_face == GL_CW && c(2) > 0) continue;

    //copy to final buffer
    memcpy(&culled[culled_last], &projected[p_id], 3*vertex_sz*sizeof(float));
    culled_last += 3*vertex_sz;
  }

  //rasterization
  #define PIXEL(i,j) (4*(i*buffer_width+j))
  #define SET_PIXEL(i,j,r,g,b) { color[PIXEL(i,j)+0] = r; \
                                 color[PIXEL(i,j)+1] = g; \
                                 color[PIXEL(i,j)+2] = b; \
                                 color[PIXEL(i,j)+3] = 255;}

   //clear color buffer
   memset((void*)color, 0, (4*buffer_width*buffer_height)*sizeof(GLubyte));
   for(int p_id = 0; p_id < culled_last; p_id += 3*vertex_sz)
   {
     #define ROUND(x) ((int)(x + 0.5f))
     struct Vertex
     {
       float x, y;
       vec3 color;
       float z, w;

       Vertex() {}

       Vertex(const float* v_packed, const mat4& vp)
       {
         //we need x and y positions mapped to the viewport and
         //with integer coordinates, otherwise we'll have displacements
         //for start and end which are huge when because of 0 < dy < 1;
         //these cases must be treated as straight, horizontal lines.
         vec4 pos = vp*vec4(v_packed[0], v_packed[1], 1.0f, 1.0f);
         x = ROUND(pos(0)); y = ROUND(pos(1));
         z = v_packed[2]; w = v_packed[3];
         color = vec3(v_packed[4], v_packed[5], v_packed[6]);
       }

       Vertex operator-(const Vertex& rhs)
       {
         Vertex out;
         out.x = x - rhs.x;
         out.y = y - rhs.y;
         out.color = color - rhs.color;
         out.z = z - rhs.z;
         out.w = w - rhs.w; //TODO: not sure if I should do this
         return out;
       }

       void operator+=(const Vertex& rhs)
       {
         x += rhs.x;
         y += rhs.y;
         color = color + rhs.color;
         z += rhs.z;
         w += rhs.w; //TODO: not sure if I should do this neither
       }

       Vertex operator/(float k)
       {
         Vertex out;
         out.x = x / k;
         out.y = y / k;
         out.color = color * (1.0f/k);
         out.z = z / k;
         out.w = w / k;
         return out;
       }
     };

     //unpack vertex data into structs so we can
     //easily interpolate/operate them.
     Vertex v0(&culled[p_id+0*vertex_sz], viewport);
     Vertex v1(&culled[p_id+1*vertex_sz], viewport);
     Vertex v2(&culled[p_id+2*vertex_sz], viewport);

     //order vertices by y coordinate
     #define SWAP(a,b) { Vertex aux = b; b = a; a = aux; }
     if( v0.y > v1.y ) SWAP(v0, v1);
     if( v0.y > v2.y ) SWAP(v0, v2);
     if( v1.y > v2.y ) SWAP(v1, v2);

     //these dVdy_ variables define how much we must
     //increment v when increasing one unit in y, so
     //we can use this to compute the start and end
     //boundaries for rasterization. Notice that not
     //only this defines the actual x coordinate of the
     //fragment in the scanline, but all the other
     //attributes. Also, notice that y is integer and
     //thus if we make dy0 = (v1.y-v0.y) steps in y, for intance,
     //incrementing v0 with dVdy0 at each step, by the end of
     //the dy steps we'll have:
     //
     // v0 + dy0 * dVdy0 = v0 + dy0*(v1-v0)/dy0 = v0 + v1 - v0 = v1
     //
     //which is exactly what we want, a linear interpolation
     //between v0 and v1 with dy0 steps
     Vertex dV_dy0 = (v1-v0)/(float)(v1.y-v0.y);
     Vertex dV_dy1 = (v2-v0)/(float)(v2.y-v0.y);
     Vertex dV_dy2 = (v2-v1)/(float)(v2.y-v1.y);
     Vertex start, end;
     Vertex dStart_dy, dEnd_dy;

     //this will tell us whether we should change dStart_dy
     //or dEnd_dy to the next active edge (dV_dy2) when we
     //reach halfway the triangle
     Vertex *next_active_edge;

     //decide start/end edges. If v1 is to the left
     //side of the edge connecting v0 and v2, then v0v1
     //is the starting edge and v0v2 is the ending edge;
     //if v1 is to the right, it is the contrary.
     //the v0v1 edge will be substituted by the v1v2 edge
     //when we reach the v1 vertex while scanlining, so we
     //store which of the start/end edges we should replace
     //with v1v2.
     vec3 right_side = vec3(v1.x-v0.x, v1.y-v0.y, 0.0f).cross(vec3(v2.x-v0.x, v2.y-v0.y, 0.0f));
     if( right_side(2) > 0.0f )
     {
       dEnd_dy = dV_dy0;
       dStart_dy = dV_dy1;
       next_active_edge = &dEnd_dy;
     }
     else
     {
       dEnd_dy = dV_dy1;
       dStart_dy = dV_dy0;
       next_active_edge = &dStart_dy;
     }

     //handle flat top triangles
     if( v0.y == v1.y )
     {
       //switch active edge and update
       //starting and ending points
       if( v0.x < v1.x )
       {
         dEnd_dy = dV_dy2;
         start = v0; end = v1;
       }
       else
       {
         dStart_dy = dV_dy2;
         start = v1; end = v0;
       }
     }
     else start = end = v0;

     //loop over scanlines
     for(int y = v0.y; y <= v2.y; ++y)
     {
       //rasterize scanline
       for(int x = ROUND(start.x); x <= ROUND(end.x); ++x)
         SET_PIXEL(y, x, 255, 255, 255);

       //switch active edges if halfway through the triangle
       //This MUST be done before incrementing, otherwise
       //once we reached v1 we would pass through it and
       //start coming back only in the next step, causing
       //the big "leaking" triangles!
       if( y == (int)v1.y ) *next_active_edge = dV_dy2;

       //increment bounds
       start += dStart_dy; end += dEnd_dy;
     }
   }

  //-------------------------------------------------------
  //---------------------- DISPLAY ------------------------
  //-------------------------------------------------------
  // send to GPU in texture unit 0
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, color_gpu);

  //WARNING: be careful with RGB pixel data
  //as OpenGL expects 4-byte aligned data
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
