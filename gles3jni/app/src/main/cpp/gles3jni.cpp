/*
 * Copyright 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "gles3jni.h"

#include <jni.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char VERTEX_SHADER[] = R"glsl(
#version 300 es
layout(location = 0) in vec2 pos;
layout(location= 1) in vec4 color;
layout(location= 2) in vec4 scaleRot;
layout(location= 3) in vec2 offset;
out vec4 vColor;
void main() {
mat2 sr = mat2(scaleRot.xy, scaleRot.zw);
gl_Position = vec4(sr*pos + offset, 0.0, 1.0);
vColor = color;
}
)glsl";

static const char FRAGMENT_SHADER[] =
    "#version 300 es\n"
    "precision mediump float;\n"
    "in vec4 vColor;\n"
    "out vec4 outColor;\n"
    "void main() {\n"
    "    outColor = vColor;\n"
    "}\n";


const Vertex QUAD[4] = {
    // Square with diagonal < 2 so that it fits in a [-1 .. 1]^2 square
    // regardless of rotation.
    {{-0.7f, -0.7f}, {0x00, 0xFF, 0x00}},
    {{0.7f, -0.7f}, {0x00, 0x00, 0xFF}},
    {{-0.7f, 0.7f}, {0xFF, 0x00, 0x00}},
    {{0.7f, 0.7f}, {0xFF, 0xFF, 0xFF}},
};

const float original_vertices_with_positions_only[] = {
    // positions
    -0.5f, -0.5f, -0.5f,
    0.5f, -0.5f, -0.5f,
    0.5f, 0.5f, -0.5f,
    0.5f, 0.5f, -0.5f,
    -0.5f, 0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f,

    -0.5f, -0.5f, 0.5f,
    0.5f, -0.5f, 0.5f,
    0.5f, 0.5f, 0.5f,
    0.5f, 0.5f, 0.5f,
    -0.5f, 0.5f, 0.5f,
    -0.5f, -0.5f, 0.5f,

    -0.5f, 0.5f, 0.5f,
    -0.5f, 0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f, 0.5f,
    -0.5f, 0.5f, 0.5f,

    0.5f, 0.5f, 0.5f,
    0.5f, 0.5f, -0.5f,
    0.5f, -0.5f, -0.5f,
    0.5f, -0.5f, -0.5f,
    0.5f, -0.5f, 0.5f,
    0.5f, 0.5f, 0.5f,

    -0.5f, -0.5f, -0.5f,
    0.5f, -0.5f, -0.5f,
    0.5f, -0.5f, 0.5f,
    0.5f, -0.5f, 0.5f,
    -0.5f, -0.5f, 0.5f,
    -0.5f, -0.5f, -0.5f,

    -0.5f, 0.5f, -0.5f,
    0.5f, 0.5f, -0.5f,
    0.5f, 0.5f, 0.5f,
    0.5f, 0.5f, 0.5f,
    -0.5f, 0.5f, 0.5f,
    -0.5f, 0.5f, -0.5f
};

bool checkGlError(const char* funcName) {
  GLint err = glGetError();
  if (err != GL_NO_ERROR) {
    ALOGE("GL error after %s(): 0x%08x\n", funcName, err);
    return true;
  }
  return false;
}

GLuint createShader(GLenum shaderType, const char* src) {
  GLuint shader = glCreateShader(shaderType);
  if (!shader) {
    checkGlError("glCreateShader");
    return 0;
  }
  glShaderSource(shader, 1, &src, NULL);

  GLint compiled = GL_FALSE;
  glCompileShader(shader);
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
  if (!compiled) {
    GLint infoLogLen = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLen);
    if (infoLogLen > 0) {
      GLchar* infoLog = (GLchar*)malloc(infoLogLen);
      if (infoLog) {
        glGetShaderInfoLog(shader, infoLogLen, NULL, infoLog);
        ALOGE("Could not compile %s shader:\n%s\n",
              shaderType == GL_VERTEX_SHADER ? "vertex" : "fragment", infoLog);
        free(infoLog);
      }
    }
    glDeleteShader(shader);
    return 0;
  }

  return shader;
}

GLuint createProgram(const char* vtxSrc, const char* fragSrc) {
  GLuint vtxShader = 0;
  GLuint fragShader = 0;
  GLuint program = 0;
  GLint linked = GL_FALSE;

  vtxShader = createShader(GL_VERTEX_SHADER, vtxSrc);
  if (!vtxShader) goto exit;

  fragShader = createShader(GL_FRAGMENT_SHADER, fragSrc);
  if (!fragShader) goto exit;

  program = glCreateProgram();
  if (!program) {
    checkGlError("glCreateProgram");
    goto exit;
  }
  glAttachShader(program, vtxShader);
  glAttachShader(program, fragShader);

  glLinkProgram(program);
  glGetProgramiv(program, GL_LINK_STATUS, &linked);
  if (!linked) {
    ALOGE("Could not link program");
    GLint infoLogLen = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLen);
    if (infoLogLen) {
      GLchar* infoLog = (GLchar*)malloc(infoLogLen);
      if (infoLog) {
        glGetProgramInfoLog(program, infoLogLen, NULL, infoLog);
        ALOGE("Could not link program:\n%s\n", infoLog);
        free(infoLog);
      }
    }
    glDeleteProgram(program);
    program = 0;
  }

exit:
  glDeleteShader(vtxShader);
  glDeleteShader(fragShader);
  return program;
}

static void printGlString(const char* name, GLenum s) {
  const char* v = (const char*)glGetString(s);
  ALOGV("GL %s: %s\n", name, v);
}

// ----------------------------------------------------------------------------

Renderer::Renderer() : mEglContext(eglGetCurrentContext()), mProgram(0), mVBState(0), mNumInstances(0), mLastFrameNs(0) {
  memset(mScale, 0, sizeof(mScale));
  memset(mAngularVelocity, 0, sizeof(mAngularVelocity));
  memset(mAngles, 0, sizeof(mAngles));
  for (int i = 0; i < VB_COUNT; i++) mVB[i] = 0;
}

Renderer::~Renderer() {
  /* The destructor may be called after the context has already been
   * destroyed, in which case our objects have already been destroyed.
   *
   * If the context exists, it must be current. This only happens when we're
   * cleaning up after a failed init().
   */
  if (eglGetCurrentContext() != mEglContext) return;
  glDeleteVertexArrays(1, &mVBState);
  glDeleteBuffers(VB_COUNT, mVB);
  glDeleteProgram(mProgram);
}

void Renderer::resize(int w, int h) {
  auto offsets = mapOffsetBuf();
  calcSceneParams(w, h, offsets);
  unmapOffsetBuf();

  // Auto gives a signed int :-(
  for (auto i = (unsigned)0; i < mNumInstances; i++) {
    mAngles[i] = drand48() * TWO_PI;
    mAngularVelocity[i] = MAX_ROT_SPEED * (2.0 * drand48() - 1.0);
  }

  mLastFrameNs = 0;

  glViewport(0, 0, w, h);
}

void Renderer::calcSceneParams(unsigned int w, unsigned int h, float* offsets) {
  // number of cells along the larger screen dimension
  const float NCELLS_MAJOR = MAX_INSTANCES_PER_SIDE;
  // cell size in scene space
  const float CELL_SIZE = 2.0f / NCELLS_MAJOR;

  // Calculations are done in "landscape", i.e. assuming dim[0] >= dim[1].
  // Only at the end are values put in the opposite order if h > w.
  const float dim[2] = {fmaxf(w, h), fminf(w, h)};
  const float aspect[2] = {dim[0] / dim[1], dim[1] / dim[0]};
  const float scene2clip[2] = {1.0f, aspect[0]};
  const int ncells[2] = {static_cast<int>(NCELLS_MAJOR),
                         (int)floorf(NCELLS_MAJOR * aspect[1])};

  float centers[2][MAX_INSTANCES_PER_SIDE];
  for (int d = 0; d < 2; d++) {
    auto offset = -ncells[d] / NCELLS_MAJOR;  // -1.0 for d=0
    for (auto i = 0; i < ncells[d]; i++) {
      centers[d][i] = scene2clip[d] * (CELL_SIZE * (i + 0.5f) + offset);
    }
  }

  int major = w >= h ? 0 : 1;
  int minor = w >= h ? 1 : 0;
  // outer product of centers[0] and centers[1]
  for (int i = 0; i < ncells[0]; i++) {
    for (int j = 0; j < ncells[1]; j++) {
      int idx = i * ncells[1] + j;
      offsets[2 * idx + major] = centers[0][i];
      offsets[2 * idx + minor] = centers[1][j];
    }
  }

  mNumInstances = ncells[0] * ncells[1];
  mScale[major] = 0.5f * CELL_SIZE * scene2clip[0];
  mScale[minor] = 0.5f * CELL_SIZE * scene2clip[1];
}

void Renderer::step() {
  ALOGV("x,y = %f,%f", xpos, ypos);

  if (first_touch_) {
    last_x_ = xpos;
    last_y_ = ypos;
    first_touch_ = false;
  }
  float xoffset = -1.0 * (xpos - last_x_);
  float yoffset = -1.0 * (ypos - last_y_);
  last_x_ = xpos;
  last_y_ = ypos;

  // 测试使用touch 的数据进行修改
  float dt = abs(xoffset) + abs(yoffset) * 0.0001f;
  ALOGV("dt = %f", dt);

  for (unsigned int i = 0; i < mNumInstances; i++) {
    mAngles[i] += mAngularVelocity[i] * dt;
    if (mAngles[i] >= TWO_PI) {
      mAngles[i] -= TWO_PI;
    } else if (mAngles[i] <= -TWO_PI) {
      mAngles[i] += TWO_PI;
    }
  }

  float* transforms = mapTransformBuf();
  for (unsigned int i = 0; i < mNumInstances; i++) {
    float s = sinf(mAngles[i]);
    float c = cosf(mAngles[i]);
    transforms[4 * i + 0] = c * mScale[0];
    transforms[4 * i + 1] = s * mScale[1];
    transforms[4 * i + 2] = -s * mScale[0];
    transforms[4 * i + 3] = c * mScale[1];
  }
  unmapTransformBuf();
}

void Renderer::render() {
  step();

  glClearColor(0.2f, 0.2f, 0.3f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  draw(mNumInstances);
  checkGlError("Renderer::render");
}

/* 可以加读写锁或者用原子数保护，当前省时间没有进行此操作 */
void Renderer::handleTouch(float x, float y) {
  xpos = x;
  ypos = y;
}
bool Renderer::init() {
  mProgram = createProgram(VERTEX_SHADER, FRAGMENT_SHADER);
  if (!mProgram) return false;

  glGenBuffers(VB_COUNT, mVB);
  glBindBuffer(GL_ARRAY_BUFFER, mVB[VB_INSTANCE]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(QUAD), &QUAD[0], GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, mVB[VB_SCALEROT]);
  glBufferData(GL_ARRAY_BUFFER, MAX_INSTANCES * 4 * sizeof(float), NULL,
               GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, mVB[VB_OFFSET]);
  glBufferData(GL_ARRAY_BUFFER, MAX_INSTANCES * 2 * sizeof(float), NULL,
               GL_STATIC_DRAW);

  glGenVertexArrays(1, &mVBState);
  glBindVertexArray(mVBState);

  glBindBuffer(GL_ARRAY_BUFFER, mVB[VB_INSTANCE]);
  glVertexAttribPointer(POS_ATTRIB, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (const GLvoid*)offsetof(Vertex, pos));
  glVertexAttribPointer(COLOR_ATTRIB, 4, GL_UNSIGNED_BYTE, GL_TRUE,
                        sizeof(Vertex), (const GLvoid*)offsetof(Vertex, rgba));
  glEnableVertexAttribArray(POS_ATTRIB);
  glEnableVertexAttribArray(COLOR_ATTRIB);

  glBindBuffer(GL_ARRAY_BUFFER, mVB[VB_SCALEROT]);
  glVertexAttribPointer(SCALEROT_ATTRIB, 4, GL_FLOAT, GL_FALSE,
                        4 * sizeof(float), 0);
  glEnableVertexAttribArray(SCALEROT_ATTRIB);
  glVertexAttribDivisor(SCALEROT_ATTRIB, 1);

  glBindBuffer(GL_ARRAY_BUFFER, mVB[VB_OFFSET]);
  glVertexAttribPointer(OFFSET_ATTRIB, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float),
                        0);
  glEnableVertexAttribArray(OFFSET_ATTRIB);
  glVertexAttribDivisor(OFFSET_ATTRIB, 1);

  ALOGV("Using OpenGL ES 3.0 renderer");
  return true;
}
float* Renderer::mapOffsetBuf() {
  glBindBuffer(GL_ARRAY_BUFFER, mVB[VB_OFFSET]);
  return (float*)glMapBufferRange(
      GL_ARRAY_BUFFER, 0, MAX_INSTANCES * 2 * sizeof(float),
      GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
}
void Renderer::unmapOffsetBuf() {
  glUnmapBuffer(GL_ARRAY_BUFFER);
}
float* Renderer::mapTransformBuf() {
  glBindBuffer(GL_ARRAY_BUFFER, mVB[VB_SCALEROT]);
  return (float*)glMapBufferRange(
      GL_ARRAY_BUFFER, 0, MAX_INSTANCES * 4 * sizeof(float),
      GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
}
void Renderer::unmapTransformBuf() {
  glUnmapBuffer(GL_ARRAY_BUFFER);
}
void Renderer::draw(unsigned int numInstances) {
  glUseProgram(mProgram);
  glBindVertexArray(mVBState);
  glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, numInstances);
}

Renderer* createES3Renderer() {
  Renderer* renderer = new Renderer;
  if (!renderer->init()) {
    delete renderer;
    return NULL;
  }
  return renderer;
}


// ----------------------------------------------------------------------------

static Renderer* g_renderer = NULL;

extern "C" {
JNIEXPORT void JNICALL Java_com_android_gles3jni_GLES3JNILib_init(JNIEnv* env,
                                                                  jobject obj);
JNIEXPORT void JNICALL Java_com_android_gles3jni_GLES3JNILib_resize(
    JNIEnv* env, jobject obj, jint width, jint height);
JNIEXPORT void JNICALL Java_com_android_gles3jni_GLES3JNILib_step(JNIEnv* env,
                                                                  jobject obj);
JNIEXPORT void JNICALL Java_com_android_gles3jni_GLES3JNILib_onTouchEventNative(
    JNIEnv* env, jobject obj, jint action, jfloat x, jfloat y);
};

#if !defined(DYNAMIC_ES3)
static GLboolean gl3stubInit() { return GL_TRUE; }
#endif

JNIEXPORT void JNICALL Java_com_android_gles3jni_GLES3JNILib_init(JNIEnv* env,
                                                                  jobject obj) {
  if (g_renderer) {
    delete g_renderer;
    g_renderer = NULL;
  }

  printGlString("Version", GL_VERSION);
  printGlString("Vendor", GL_VENDOR);
  printGlString("Renderer", GL_RENDERER);
  printGlString("Extensions", GL_EXTENSIONS);

  const char* versionStr = (const char*)glGetString(GL_VERSION);
  if (strstr(versionStr, "OpenGL ES 3.") && gl3stubInit()) {
    g_renderer = createES3Renderer();
  } else {
    ALOGE("Unsupported OpenGL ES version");
  }
}

JNIEXPORT void JNICALL Java_com_android_gles3jni_GLES3JNILib_resize(
    JNIEnv* env, jobject obj, jint width, jint height) {
  if (g_renderer) {
    g_renderer->resize(width, height);
  }
}

JNIEXPORT void JNICALL Java_com_android_gles3jni_GLES3JNILib_step(JNIEnv* env,
                                                                  jobject obj) {
  if (g_renderer) {
    g_renderer->render();
  }
}

JNIEXPORT void JNICALL Java_com_android_gles3jni_GLES3JNILib_onTouchEventNative(
    JNIEnv* env, jobject obj, jint action, jfloat x, jfloat y) {
  // 简单的跨线程，
  float xpos = static_cast<float>(x);
  float ypos = static_cast<float>(y);
  if (g_renderer) {
    g_renderer->handleTouch(x, y);
  }
}
