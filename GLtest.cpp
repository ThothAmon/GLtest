#include <iostream>
#include <fstream>
#include <cassert>
#include <vector>
#include <Ecore_X.h>
#include <unistd.h>
#include "math_3d.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>

#define GL_FRAGMENT_SHADER                               0x8B30
#define GL_VERTEX_SHADER                                 0x8B31
#define GL_COMPILE_STATUS                                0x8B81
#define GL_LINK_STATUS                                   0x8B82
#define GL_INFO_LOG_LENGTH                               0x8B84

#define DALI_GLES_VERSION 30
#define DALI_COMPOSE_SHADER(STR) #STR

typedef unsigned int     GLenum;
typedef int8_t           GLbyte;
typedef int              GLint;
typedef unsigned int     GLuint;

GLint width = 1280, height = 1024;
GLuint programObject;
EGLDisplay EglDisplay;
EGLSurface surface_one, surface_two;
GLuint mId; ///< Id of the texture

GLuint VBO, IBO;
GLuint gWVPLocation, gSampler;

struct Vertex
{
  Vector3f m_pos;
  Vector2f m_tex;

  Vertex() {}

  Vertex(Vector3f pos, Vector2f tex)
  {
      m_pos = pos;
      m_tex = tex;
  }
};

static GLuint shader = 0;

void DoGraphicsSetup(void);
void Draw(EGLSurface surface);
char *LoadTexture(void);
void CreateTexture(char *buffer);
void CreateVertexBuffer(void);
void CreateIndexBuffer(void);
void CompileShaders(void);
void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType);

int main(int argc, char **argv)
{
  if(!ecore_x_init(NULL))
  {
    std::cout << "Failed to initialise Ecore X" << std::endl;
  }
  std::cout << "Hello OpenGL World" << std::endl;

  Ecore_X_Display* display = ecore_x_display_get();
  Ecore_X_Screen* screen = ecore_x_default_screen_get();

  EglDisplay = eglGetDisplay((EGLNativeDisplayType)display);
  EGLint error = eglGetError();
  if(EglDisplay == NULL && error != EGL_SUCCESS)
  {
    std::cout << "Failed to get EGL Display" << std::endl;
  }

  EGLint majorVersion = 0;
  EGLint minorVersion = 0;
  if ( !eglInitialize( EglDisplay, &majorVersion, &minorVersion ) )
  {
    std::cout << "Failed to initialise EGL" << std::endl;
    return false;
  }
  eglBindAPI(EGL_OPENGL_ES_API);

  std::vector<EGLint> configAttribs;
  configAttribs.push_back( EGL_SURFACE_TYPE );
  configAttribs.push_back( EGL_WINDOW_BIT );

  configAttribs.push_back( EGL_RENDERABLE_TYPE );

  configAttribs.push_back( EGL_OPENGL_ES3_BIT_KHR );
  configAttribs.push_back( EGL_RED_SIZE );
  configAttribs.push_back( 8 );
  configAttribs.push_back( EGL_GREEN_SIZE );
  configAttribs.push_back( 8 );
  configAttribs.push_back( EGL_BLUE_SIZE );
  configAttribs.push_back( 8 );

  configAttribs.push_back( EGL_ALPHA_SIZE );
  configAttribs.push_back( 0 );
  configAttribs.push_back( EGL_DEPTH_SIZE );
  configAttribs.push_back( 24 );
  configAttribs.push_back( EGL_STENCIL_SIZE );
  configAttribs.push_back( 8 );
  configAttribs.push_back( EGL_NONE );

  EGLConfig EglConfig;
  EGLint numConfigs;

  if ( eglChooseConfig( EglDisplay, &(configAttribs[0]), &EglConfig, 1, &numConfigs ) != EGL_TRUE )
  {
    EGLint error = eglGetError();
    std::cout << "Failed to choose EGL config: " << error << std::endl;
  }

  if ( numConfigs != 1 )
  {
    std::cout << "No configurations found" << std::endl;
  }

  std::vector<EGLint> ContextAttribs;
  ContextAttribs.push_back( EGL_CONTEXT_MAJOR_VERSION_KHR );
  ContextAttribs.push_back( DALI_GLES_VERSION / 10 );
  ContextAttribs.push_back( EGL_CONTEXT_MINOR_VERSION_KHR );
  ContextAttribs.push_back( DALI_GLES_VERSION % 10 );
  ContextAttribs.push_back( EGL_CONTEXT_PRIORITY_LEVEL_IMG );
  ContextAttribs.push_back( EGL_CONTEXT_PRIORITY_HIGH_IMG );
  ContextAttribs.push_back( EGL_NONE );

  /********** Resource Context (used for holding assets such as textures) **********/
  EGLContext EglResourceContext = eglCreateContext(EglDisplay, EglConfig, EGL_NO_CONTEXT, &(ContextAttribs[0]));
  if(eglGetError() != 0x3000)
  {
    std::cout << "Resource context not created" << std::endl;
  }
  else
  {
    std::cout << "Got Resource context: " << EglResourceContext << std::endl;
  }

  if( eglMakeCurrent( EglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EglResourceContext ) == EGL_FALSE)
  {
    std::cout << "Failed to bind the Resource context" << std::endl;
  }
  else
  {
    EGLContext current = eglGetCurrentContext();
    std::cout << "Got current context: " << current << std::endl;
  }

  /*********************************************************************************/

  DoGraphicsSetup(); // Create the Shader and bind to the resource context

  /*** Render the first window ***/
  EGLContext EglContextOne = eglCreateContext(EglDisplay, EglConfig, EglResourceContext, &(ContextAttribs[0]));
  if(eglGetError() != 0x3000)
  {
    std::cout << "First context not created" << std::endl;
  }
  else
  {
    std::cout << "Got first context: " << EglContextOne << std::endl;
  }

  Ecore_X_Window window_one = ecore_x_window_new( 0, 0, 0, width, height);
  if(!window_one)
  {
    std::cout << "Failed to create window one" << std::endl;
  }

  surface_one = eglCreateWindowSurface( EglDisplay, EglConfig, (EGLNativeWindowType)window_one, NULL );
  if(eglGetError() != 0x3000)
  {
    std::cout << "No Surface created" << std::endl;
  }

  /******* Second Window *********/
  EGLContext EglContextTwo = eglCreateContext(EglDisplay, EglConfig, EglResourceContext, &(ContextAttribs[0]));
  if(eglGetError() != 0x3000)
  {
    std::cout << "Second context not created" << std::endl;
  }
  else
  {
    std::cout << "Got EglContextTwo context: " << EglContextTwo << std::endl;
  }

  Ecore_X_Window window_two = ecore_x_window_new( 0, 0, 0, width, height);
  if(!window_two)
  {
    std::cout << "Failed to create window two" << std::endl;
  }

  ecore_x_icccm_title_set(window_two, "Looking through the mirror...");
  ecore_x_window_area_clear(window_two, 0, 0, width, height);
  ecore_x_window_show(window_two);

  if(ecore_x_window_visible_get(window_two))
  {
    std::cout << "Window should be visible" << std::endl;
  }

  surface_two = eglCreateWindowSurface( EglDisplay, EglConfig, (EGLNativeWindowType)window_two, NULL );
  if(eglGetError() != 0x3000)
  {
    std::cout <<"No Surface created" << std::endl;
  }

  while(1)
  {
    if(eglMakeCurrent( EglDisplay, surface_one, surface_one, EglContextOne ) == EGL_FALSE)
    {
      std::cout << "Failed to bind the first context to the rendering thread" << std::endl;
    }
    else
    {
      EGLContext current = eglGetCurrentContext();
      std::cout << "Got current context: " << current << std::endl;
    }

    ecore_x_icccm_title_set(window_one, "Through the looking glass...");
    ecore_x_window_area_clear(window_one, 0, 0, width, height);
    ecore_x_window_show(window_one);

    if(ecore_x_window_visible_get(window_one))
    {
      std::cout << "Window should be visible" << std::endl;
    }

    glClearColor(0.0f, 0.0f, 0.5f, 0.0f);
    glUseProgram(shader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mId);

    Draw(surface_one); // Render the first window */

    if(eglMakeCurrent( EglDisplay, surface_two, surface_two, EglContextTwo ) == EGL_FALSE)
    {
      std::cout << "Failed to bind the second context to the rendering thread" << std::endl;
    }
    else
    {
      EGLContext current = eglGetCurrentContext();
      std::cout << "Got current context: " << current << std::endl;
    }

    glClearColor(0.5f, 0.0f, 0.0f, 0.0f);
    glUseProgram(shader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mId);

    Draw(surface_two); // Render the second window */
  }

  /*** OpenGL shutdown ***/
  eglMakeCurrent( EglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );

  eglDestroySurface(EglDisplay, surface_one);
  eglDestroySurface(EglDisplay, surface_two);

  eglDestroyContext(EglDisplay, EglResourceContext);
  eglDestroyContext(EglDisplay, EglContextOne);
  eglDestroyContext(EglDisplay, EglContextTwo);
  eglTerminate(EglDisplay);

  ecore_x_window_free(window_one);
  ecore_x_window_free(window_two);

  ecore_x_shutdown();
  std::cout << "Shutdown OpenGL, EcoreX and exiting..." << std::endl;

	return 0;
}

void DoGraphicsSetup(void)
{
  CreateVertexBuffer();
  CreateIndexBuffer();
  CompileShaders();

  glUniform1i(gSampler, 0);

  /*** Load Textures ***/
  char *texturebuf = LoadTexture();
  CreateTexture(texturebuf);

  if(!eglWaitGL())
  {
    std::cout << "eglWaitGL failed" << std::endl;
  }
}

GLuint LoadShader(const char *shaderSrc, GLenum type)
{
  GLuint shader;
  GLint compiled;
  GLint Lengths[1];

  Lengths[0] = strlen(shaderSrc);
  std::cout << "Shader length is: " << Lengths[0] << std::endl;

  // Create the shader object
  shader = glCreateShader(type);
  if(shader == 0)
  {
     return 0;
  }

  // Load the shader source
  glShaderSource(shader, 1, &shaderSrc, Lengths);

  // Compile the shader
  glCompileShader(shader);

  // Check the compile status
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

  if(!compiled)
  {
     GLint infoLen = 0;
     glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);

     if(infoLen > 1)
     {
        char* infoLog = (char *)malloc(sizeof(char) * infoLen);
        glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
        std::cout << "Error compiling shader" << infoLog << std::endl;
        free(infoLog);
        exit(1);
     }
     glDeleteShader(shader);
     return 0;
  }
  return shader;
}

// Draw an object using the shader pair created in Init()
void Draw(EGLSurface surface)
{
  // Clear the color buffer
  glClear(GL_COLOR_BUFFER_BIT);

  // Load the vertex data
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)12);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
  glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, 0);

  eglSwapBuffers(EglDisplay, surface);
}

char *LoadTexture(void)
{
  char *buffer = NULL;
  std::ifstream texturefile ("/home/SERILOCAL/jason.perry/texture.dat", std::ios::binary);

  if(texturefile.is_open())
  {
    texturefile.seekg(0, texturefile.end);
    std::streampos size = texturefile.tellg();
    texturefile.seekg(0, texturefile.beg);
    std::cout << "Size of file is: " << size << std::endl;

    buffer = new char[size];
    if(buffer == nullptr)
    {
      std::cout << "Unable to allocate memory for texture buffer" << std::endl;
    }

    texturefile.seekg(0, std::ios::beg);
    texturefile.read(buffer, size);
    texturefile.close();
  }
  else
  {
    std::cout << "Unable to read texture file into memory" << std::endl;
  }

  return buffer;
}

void CreateTexture(char *buffer)
{
  GLsizei n = 1;
  GLuint* textures;
  GLenum target = GL_TEXTURE_2D;
  GLuint framebuffer = 0;
  GLenum format = GL_RGBA, type = GL_UNSIGNED_BYTE;

  /*
   * After creating the handle and loading the texture data and parameters you can simply switch textures on the fly by binding different handles into the OpenGL state.
   * You no longer need to load the data again. From now on it is the job of the OpenGL driver to make sure the data is loaded in time to the GPU before rendering starts.
   */
  glGenTextures(n, &mId); // The OpenGL method can be used to generate multiple handles at the same time; here we generate just one.
  std::cout << "Handles: " << mId << std::endl;

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, mId);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glTexImage2D(GL_TEXTURE_2D, 0, format, 480, 480, 0, format, type, buffer);
}

void CreateVertexBuffer()
{
  Vertex Vertices[4] = { Vertex(Vector3f(-1.0f, -1.0f, 0.5773f), Vector2f(0.0f, 0.0f)),
                           Vertex(Vector3f(0.0f, -1.0f, -1.15475), Vector2f(0.5f, 0.0f)),
                           Vertex(Vector3f(1.0f, -1.0f, 0.5773f),  Vector2f(1.0f, 0.0f)),
                           Vertex(Vector3f(0.0f, 1.0f, 0.0f),      Vector2f(0.5f, 1.0f)) };

  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);
}

void CreateIndexBuffer()
{
  unsigned int Indices[] = { 0, 3, 1,
                             1, 3, 2,
                             2, 3, 0,
                             1, 2, 0 };

  glGenBuffers(1, &IBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices), Indices, GL_STATIC_DRAW);
}

void CompileShaders()
{
  const GLbyte pVS[] =
    "#version 300 es                                 \n"
    "layout (location = 0) in mediump vec3 Position; \n"
    "layout (location = 1) in mediump vec2 TexCoord; \n"
    "uniform mediump mat4 gWVP;                      \n"
    "out mediump vec2 TexCoord0;                     \n"
    "void main()                                     \n"
    "{                                               \n"
    "  gl_Position = vec4(Position, 1.0);            \n"
    "  TexCoord0 = TexCoord;                         \n"
    "}                                               \n";

  const GLbyte pFS[] =
    "#version 300 es                                  \n"
    "in mediump vec2 TexCoord0;                       \n"
    "out mediump vec4 FragColor;                      \n"
    "uniform sampler2D gSampler;                      \n"
    "void main()                                      \n"
    "{                                                \n"
    "  FragColor = texture(gSampler, TexCoord0);      \n"
    "}                                                \n";

  shader = glCreateProgram();

  if (shader == 0) {
    fprintf(stderr, "Error creating shader program\n");
     exit(1);
  }

  AddShader(shader, pVS, GL_VERTEX_SHADER);
  AddShader(shader, pFS, GL_FRAGMENT_SHADER);

  GLuint value = 0;
  GLchar ErrorLog[1024] = { 0 };

  glLinkProgram(shader);
  glGetProgramiv(shader, GL_LINK_STATUS, &value);

  if (value == 0) {
    glGetProgramInfoLog(shader, sizeof(ErrorLog), NULL, ErrorLog);
    fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
    exit(1);
  }

  glValidateProgram(shader);
  glGetProgramiv(shader, GL_VALIDATE_STATUS, &value);

  if (!value) {
      glGetProgramInfoLog(shader, sizeof(ErrorLog), NULL, ErrorLog);
      fprintf(stderr, "Invalid shader program: '%s'\n", ErrorLog);
      exit(1);
  }

  glUseProgram(shader);

  gWVPLocation = glGetUniformLocation(shader, "gWVP");
  //assert(gWVPLocation != 0xFFFFFFFF);
  gSampler = glGetUniformLocation(shader, "gSampler");
  //assert(gSampler != 0xFFFFFFFF);
}

void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
  GLuint ShaderObj = glCreateShader(ShaderType);

  if (ShaderObj == 0) {
    fprintf(stderr, "Error creating shader type %d\n", ShaderType);
    exit(0);
  }

  const GLchar* p[1];
  p[0] = pShaderText;
  GLint Lengths[1];
  Lengths[0]= strlen(pShaderText);

  glShaderSource(ShaderObj, 1, p, Lengths);
  glCompileShader(ShaderObj);

  GLint success;
  glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);

  if (!success) {
    GLchar InfoLog[1024];
    glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
    fprintf(stderr, "Error compiling shader type %d: '%s'\n", ShaderType, InfoLog);
    exit(1);
  }

  glAttachShader(ShaderProgram, ShaderObj);
}

/* EOF */
