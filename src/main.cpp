#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <ctime>
#include <iostream>

#include <nanogui/opengl.h>
#include <nanogui/glutil.h>
#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <nanogui/layout.h>
#include <nanogui/button.h>
#include <nanogui/slider.h>
#include <nanogui/label.h>
#include <nanogui/checkbox.h>
#include <nanogui/colorpicker.h>
#include <nanogui/combobox.h>

#include "../include/mesh.h"
#include "../include/ogl.h"
#include "../include/param.h"
#include "../include/matrix.h"

#define THETA 0.0174533f
#define COSTHETA float(cos(THETA))
#define SINTHETA float(sin(THETA))

class ExampleApp : public nanogui::Screen
{
private:
  nanogui::GLShader mShader;
  Mesh mMesh;
  OGL *mOGL;

  nanogui::Label *framerate_open;
  nanogui::Label *framerate_almost;

  GlobalParameters param;

  //vertex buffers
  int n_vertices, vertex_sz;
  float *vbuffer, *clipped, *culled, *projected;

  //pixel buffers
  int buffer_height, buffer_width;
  GLubyte *color; float *depth;

  GLuint color_gpu;

public:
  ExampleApp(const char* path) : nanogui::Screen(Eigen::Vector2i(960, 540), "NanoGUI Test")
  {
    //----------------------------------
    //----------- GUI setup ------------
    //----------------------------------
    using namespace nanogui;

    Window* window = new Window(this, "Scene options");
    window->setPosition(Vector2i(0, 0));
    window->setLayout(new GroupLayout());

    Button *reset_view = new Button(window, "Reset view");
    reset_view->setTooltip("Reset view so the object will be centered again");
    reset_view->setCallback( [this] { param.cam.up = glm::vec3(0.0f, 1.0f, 0.0f);
                                      param.cam.look_dir = glm::vec3(0.0f, 0.0f, -1.0f);
                                      param.cam.right = glm::vec3(1.0f, 0.0f, 0.0f);
                                      param.cam.eye = glm::vec3(0.0f, 0.0f, 0.0f);
                                      param.cam.near = 1.0f; param.cam.far = 10.0f;
                                      param.cam.FoVy = 45.0f; param.cam.FoVx = 45.0f; });

    new Label(window, "Near Z plane", "sans-bold");

    Slider *near_plane = new Slider(window);
    near_plane->setFixedWidth(100);
    near_plane->setTooltip("Set near Z plane to any value between 0 and 20");
    near_plane->setCallback( [this](float val) { param.cam.near = val * 20.0f; } );

    new Label(window, "Far Z plane", "sans-bold");

    Slider *far_plane = new Slider(window);
    far_plane->setFixedWidth(100);
    far_plane->setTooltip("Set near Z plane to any value between 10 and 100");
    far_plane->setCallback( [this](float val) { param.cam.far = 10.0f + val * (100.0f - 10.0f); } );

    new Label(window, "Field of view y (deg)", "sans-bold");

    Slider *fovy = new Slider(window);
    fovy->setFixedWidth(100);
    fovy->setTooltip("Set the field of view in Y to any value between 2 and 150");
    fovy->setCallback( [this](float val) { param.cam.FoVy = 2.0f + val * (150.0f - 2.0f); } );

    new Label(window, "Field of view x (deg)", "sans-bold");

    Slider *fovx = new Slider(window);
    fovx->setFixedWidth(100);
    fovx->setTooltip("Set the field of view in x to any value between 2 and 150");
    fovx->setCallback( [this](float val) { param.cam.FoVx = 2.0f + val * (150.0f - 2.0f); } );

    new Label(window, "Model color", "sans-bold");
    ColorPicker *color_picker = new ColorPicker(window, param.model_color);
    color_picker->setFinalCallback([this](const Color& c) { this->param.model_color = c.head<3>(); });

    CheckBox *draw_cw = new CheckBox(window, "Draw triangles in CW order");
    draw_cw->setTooltip("Uncheck this box for drawing triangles in CCW order");
    draw_cw->setCallback([&](bool cw) { param.front_face = cw ? GL_CW : GL_CCW; });

    CheckBox *lock_view = new CheckBox(window, "Lock view on the model");
    lock_view->setTooltip("Lock view point at the point where the model is centered. This will disable camera rotation.");
    lock_view->setCallback([&](bool lock) { param.cam.lock_view = lock;
                                            param.cam.look_dir = glm::normalize(glm::vec3(0.0f, 0.0f, -5.5f) - param.cam.eye);
                                            param.cam.right = glm::cross(param.cam.look_dir, param.cam.up);
                                          });

    ComboBox *draw_mode = new ComboBox(window, {"Points", "Wireframe", "Fill"});
    draw_mode->setCallback([&](int opt) {
                            switch(opt)
                            {
                              case 0: param.draw_mode = GL_POINT; break;
                              case 1: param.draw_mode = GL_LINE; break;
                              case 2: param.draw_mode = GL_FILL; break;
                            } });

    ComboBox *shading_model = new ComboBox(window, {"GouraudAD", "GouraudADS", "PhongADS", "No shading"});
    shading_model->setCallback([&](int opt) {
                                switch(opt)
                                {
                                  case 0: param.shading = 0; break;
                                  case 1: param.shading = 1; break;
                                  case 2: param.shading = 2; break;
                                  case 3: param.shading = 3; break;
                                } });

    //display framerates
    framerate_open = new Label(window, "framerate");
    framerate_almost = new Label(window, "framerate");

    Window *winOpenGL = new Window(this, "OpenGL");
    winOpenGL->setSize({480, 270});
    winOpenGL->setPosition(Eigen::Vector2i(50,50));
    winOpenGL->setLayout(new GroupLayout());

    mOGL = new OGL(param, path, winOpenGL);
    mOGL->setSize({480, 270});

    performLayout();

    //----------------------------------------
    //----------- Geometry loading -----------
    //----------------------------------------
    mMesh.load_file(path);

    mMesh.transform_to_center(param.model2world);
    param.model_color<<0.0f, 1.0f, 0.0f;

    param.cam.eye = glm::vec3(0.0f, 0.0f, 0.0f);
    param.cam.look_dir = glm::vec3(0.0f, 0.0f, -1.0f);
    param.cam.up = glm::vec3(0.0f, 1.0f, 0.0f);
    param.cam.right = glm::vec3(1.0f, 0.0f, 0.0f);
    param.cam.near = 1.0f; param.cam.far = 10.0f;
    param.cam.step = 0.1f;
    param.cam.FoVy = 45.0f, param.cam.FoVx = 45.0f;
    param.cam.lock_view = false;

    //light position
    param.light<<0.0f, 0.0f, 0.0f;

    //a front-facing triangle has
    //counter-clockwisely ordered vertices
    param.front_face = GL_CCW;

    //draw as filled polygons
    param.draw_mode = GL_POINTS;

    param.shading = 0;

    //--------------------------------------
    //----------- Shader options -----------
    //--------------------------------------
    mShader.initFromFiles("almostgl",
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

    mShader.bind();
    mShader.uploadAttrib<Eigen::MatrixXf>("quad_pos", quad);
    mShader.uploadAttrib<Eigen::MatrixXf>("quad_uv", texcoord);

    //we need 8 floats per vertex (4 -> XYZW, 3 -> RGB, 1 -> 1.0)
    //Normals won't be forwarded out of vertex processing
    //stage, so we don't need to store them in the vertex
    //buffer.
    //The 1.0 attribute is used for storing the 1/w value
    //we need to compute a perspectively correct interpolation
    //of the fragments
    vertex_sz = 4 + 4;
    n_vertices = mMesh.mPos.cols();

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
    buffer_height = this->height(); buffer_width = this->width();
    int n_pixels = buffer_width * buffer_height;

    //AlmostGL buffers
    color = new GLubyte[4*n_pixels];
    for(int i = 0; i < n_pixels*4; i += 4) color[i] = 0;
    for(int i = 1; i < n_pixels*4; i += 4) color[i] = 0;
    for(int i = 2; i < n_pixels*4; i += 4) color[i] = 30;
    for(int i = 3; i < n_pixels*4; i += 4) color[i] = 255;

    depth = new float[n_pixels];

    //GPU target color buffer
    glGenTextures(1, &color_gpu);
    glBindTexture(GL_TEXTURE_2D, color_gpu);
    glTexStorage2D(GL_TEXTURE_2D,
                    1,
                    GL_RGBA8,
                    buffer_width,
                    buffer_height);
  }

  virtual void draw(NVGcontext *ctx)
  {
    Screen::draw(ctx);
  }

  virtual bool keyboardEvent(int key, int scancode, int action, int modifiers) {
    if (Screen::keyboardEvent(key, scancode, action, modifiers)) return true;

    //camera movement
    if(key == GLFW_KEY_A && action == GLFW_REPEAT) {
      param.cam.eye += (-param.cam.right) * param.cam.step;
      if(param.cam.lock_view)
      {
        param.cam.look_dir = glm::normalize(glm::vec3(0.0f, 0.0f, -5.5f) - param.cam.eye);
        param.cam.right = glm::cross(param.cam.look_dir, param.cam.up);
      }
      return true;
    }
    if(key == GLFW_KEY_D && action == GLFW_REPEAT) {
      param.cam.eye += param.cam.right * param.cam.step;
      if(param.cam.lock_view)
      {
        param.cam.look_dir = glm::normalize(glm::vec3(0.0f, 0.0f, -5.5f) - param.cam.eye);
        param.cam.right = glm::cross(param.cam.look_dir, param.cam.up);
      }
      return true;
    }
    if( key == GLFW_KEY_W && action == GLFW_REPEAT ) {
      param.cam.eye += param.cam.look_dir * param.cam.step;
      //if(param.cam.lock_view) param.cam.look_dir = glm::normalize(glm::vec3(0.0f, 0.0f, -5.5f) - param.cam.eye);
      return true;
    }
    if( key == GLFW_KEY_S && action == GLFW_REPEAT ) {
      param.cam.eye += param.cam.look_dir * (-param.cam.step);
      //if(param.cam.lock_view) param.cam.look_dir = glm::normalize(glm::vec3(0.0f, 0.0f, -5.5f) - param.cam.eye);
      return true;
    }
    if( key == GLFW_KEY_R && action == GLFW_REPEAT ) {
      param.cam.eye += param.cam.up * param.cam.step;
      if(param.cam.lock_view)
      {
        param.cam.look_dir = glm::normalize(glm::vec3(0.0f, 0.0f, -5.5f) - param.cam.eye);
        param.cam.up = glm::cross(param.cam.right, param.cam.look_dir);
      }

      return true;
    }
    if( key == GLFW_KEY_F && action == GLFW_REPEAT ) {
      param.cam.eye += (-param.cam.up) * param.cam.step;
      if(param.cam.lock_view)
      {
        param.cam.look_dir = glm::normalize(glm::vec3(0.0f, 0.0f, -5.5f) - param.cam.eye);
        param.cam.up = glm::cross(param.cam.right, param.cam.look_dir);
      }
      return true;
    }

    //TODO: we can precompute sin and cos values!
    if( key == GLFW_KEY_U && action == GLFW_REPEAT ) {
      if(param.cam.lock_view) return true;

      glm::vec3 u = param.cam.look_dir, v = param.cam.up;
      param.cam.look_dir = COSTHETA*u + SINTHETA*v;
      param.cam.up = -SINTHETA*u + COSTHETA*v;

      return true;
    }
    if( key == GLFW_KEY_J && action == GLFW_REPEAT ) {
      if(param.cam.lock_view) return true;

      glm::vec3 u = param.cam.look_dir, v = param.cam.up;
      param.cam.look_dir = COSTHETA*u + -SINTHETA*v;
      param.cam.up = SINTHETA*u + COSTHETA*v;

      return true;
    }
    if( key == GLFW_KEY_K && action == GLFW_REPEAT ) {
      if(param.cam.lock_view) return true;

      glm::vec3 u = param.cam.right, v = param.cam.look_dir;
      param.cam.right = COSTHETA*u + -SINTHETA*v;
      param.cam.look_dir = SINTHETA*u + COSTHETA*v;

      return true;
    }
    if( key == GLFW_KEY_H && action == GLFW_REPEAT ) {
      if(param.cam.lock_view) return true;

      glm::vec3 u = param.cam.right, v = param.cam.look_dir;
      param.cam.right = COSTHETA*u + SINTHETA*v;
      param.cam.look_dir = -SINTHETA*u + COSTHETA*v;

      return true;
    }

    //------------
    if( key == GLFW_KEY_M && action == GLFW_REPEAT ) {
      if(param.cam.lock_view) return true;

      glm::vec3 u = param.cam.right, v = param.cam.up;
      param.cam.right = COSTHETA*u + -SINTHETA*v;
      param.cam.up = SINTHETA*u + COSTHETA*v;

      return true;
    }
    if( key == GLFW_KEY_N && action == GLFW_REPEAT ) {
      if(param.cam.lock_view) return true;

      glm::vec3 u = param.cam.right, v = param.cam.up;
      param.cam.right = COSTHETA*u + SINTHETA*v;
      param.cam.up = -SINTHETA*u + COSTHETA*v;

      return true;
    }
    //---------------

    return false;
  }

  virtual bool resizeEvent(const Eigen::Vector2i &size) override
  {
    buffer_height = this->height(); buffer_width = this->width();
    int n_pixels = buffer_width * buffer_height;

    //delete previous texture and allocate a new one with the new size
    glDeleteTextures(1, &color_gpu);
    glGenTextures(1, &color_gpu);
    glBindTexture(GL_TEXTURE_2D, color_gpu);
    glTexStorage2D(GL_TEXTURE_2D,
                    1,
                    GL_RGBA8,
                    buffer_width,
                    buffer_height);

    //TODO: this is EXTREMELY slow! the best workaround would be
    //to use std::vector which is able to do some smart resizing,
    //so it doesn't need to copy data around in the case where
    //we can just extend or shrink memory
    delete[] color;
    color = new GLubyte[4*n_pixels];

    delete[] depth;
    depth = new float[n_pixels];
  }

  virtual void drawContents()
  {
    using namespace nanogui;
    clock_t start = clock();

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
      Eigen::Vector3f v = mMesh.mPos.col(v_id);
      Eigen::Vector3f n = mMesh.mNormal.col(v_id);

      //transform vertices using model view proj.
      //notice that this is akin to what we do in
      //vertex shader.
      vec4 v_world = model2world * vec4(v(0),v(1),v(2),1.0f);
      vec4 n_world = vec4(n(0),n(1),n(2),0.0f); //TODO: use inv(trans(model2world))!
      vec4 v_out = vp * v_world;

      //compute color of this vertex using phong lighting model
      vec4 v2l = (light - v_world).unit();
      vec4 v2e = (vec4(eye, 1.0f) - v_world).unit();
      vec4 h = (v2l+v2e).unit();

      float diff = std::max(0.0f, v2l.dot(-n_world));
      float spec = std::max(0.0f, (float)pow(h.dot(-n_world), 15.0f));
      float amb = 0.2f;

      vec3 v_color;
      switch(param.shading)
      {
        case 0:
          v_color = model_color * (amb + diff);
          break;
        case 1:
          v_color = model_color * (amb + diff) + vec3(1.0f, 1.0f, 1.0f) * spec;
          break;
        case 3:
          v_color = model_color;
          break;
        default:
          v_color = model_color * (amb + diff) + vec3(1.0f, 1.0f, 1.0f) * spec;
          break;
      }

      //copy to vbuffer -> forward to next stage
      for(int i = 0; i < 4; ++i) vbuffer[vertex_sz*v_id+i] = v_out(i);
      for(int i = 0; i < 3; ++i) vbuffer[vertex_sz*v_id+(4+i)] = v_color(i);
      vbuffer[vertex_sz*v_id+7] = 1.0f;
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
      projected[projected_last+4] = clipped[v_id+4]/w;
      projected[projected_last+5] = clipped[v_id+5]/w;
      projected[projected_last+6] = clipped[v_id+6]/w;
      projected[projected_last+7] = 1.0f/w;

      //we'll still keep all the 8 floats for simplicity!
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

     //clear color and depth buffers
     memset((void*)color, 0, (4*buffer_width*buffer_height)*sizeof(GLubyte));
     for(int i = 0; i < buffer_width*buffer_height; ++i) depth[i] = 2.0f;

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
           color = vec3(v_packed[4], v_packed[5], v_packed[6]);
           z = v_packed[2]; w = v_packed[7];
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
         int s = ROUND(start.x), e = ROUND(end.x);
         Vertex dV_dx = (end - start)/(e - s);
         Vertex f = start;

         for(int x = s; x <= e; ++x)
         {
           //in order to draw only the edges, we skip this
           //the scanline rasterization in all points but
           //the extremities
           if(param.draw_mode == GL_LINE && (x != s && x != e))
            continue;

           if( f.z < depth[y*buffer_width+x] )
           {
             depth[y*buffer_width+x] = f.z;

             vec3 c = f.color * (1.0f / f.w);

             int R = std::min(255, (int)(c(0)*255.0f));
             int G = std::min(255, (int)(c(1)*255.0f));
             int B = std::min(255, (int)(c(2)*255.0f));

             SET_PIXEL(y, x, R, G, B);
           }

           f += dV_dx;
         }

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
    glTexSubImage2D(GL_TEXTURE_2D,
                    0, 0, 0,
                    buffer_width,
                    buffer_height,
                    GL_RGBA,
                    GL_UNSIGNED_BYTE,
                    color);

    //WARNING: IF WE DON'T SET THIS IT WON'T WORK!
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    mShader.bind();
    mShader.setUniform("frame", 0);

    //draw stuff
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    mShader.drawArray(GL_TRIANGLES, 0, 6);

    //framerate
    start = clock() - start;
    framerate_almost->setCaption( "AlmostGL: " + std::to_string(CLOCKS_PER_SEC/(float)start) );
    framerate_open->setCaption( "OpenGL: " + std::to_string(mOGL->framerate) );
  }
};

int main(int argc, char** args)
{
  nanogui::init();

  /* scoped variables. why this? */ {
    nanogui::ref<ExampleApp> app = new ExampleApp(args[1]);
    app->drawAll();
    app->setVisible(true);
    nanogui::mainloop();
  }

  nanogui::shutdown();
}
