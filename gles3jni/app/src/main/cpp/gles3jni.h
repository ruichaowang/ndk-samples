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

#ifndef GLES3JNI_H
#define GLES3JNI_H 1

#include <android/log.h>
#include <math.h>

#if DYNAMIC_ES3
#include "gl3stub.h"
#else
// Include the latest possible header file( GL version header )
#if __ANDROID_API__ >= 24
#include <GLES3/gl32.h>
#elif __ANDROID_API__ >= 21
#include <GLES3/gl31.h>
#else
#include <GLES3/gl3.h>
#endif

#endif

#define DEBUG 1

#define LOG_TAG "GLES3JNI"
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#if DEBUG
#define ALOGV(...) \
  __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#else
#define ALOGV(...)
#endif

// ----------------------------------------------------------------------------
// Types, functions, and data used by both ES2 and ES3 renderers.
// Defined in gles3jni.cpp.

#define MAX_INSTANCES_PER_SIDE 16
#define MAX_INSTANCES (MAX_INSTANCES_PER_SIDE * MAX_INSTANCES_PER_SIDE)
#define TWO_PI (2.0 * M_PI)
#define MAX_ROT_SPEED (0.3 * TWO_PI)

// This demo uses three coordinate spaces:
// - The model (a quad) is in a [-1 .. 1]^2 space
// - Scene space is either
//    landscape: [-1 .. 1] x [-1/(2*w/h) .. 1/(2*w/h)]
//    portrait:  [-1/(2*h/w) .. 1/(2*h/w)] x [-1 .. 1]
// - Clip space in OpenGL is [-1 .. 1]^2
//
// Conceptually, the quads are rotated in model space, then scaled (uniformly)
// and translated to place them in scene space. Scene space is then
// non-uniformly scaled to clip space. In practice the transforms are combined
// so vertices go directly from model to clip space.

#include <EGL/egl.h>
#include "gles3jni.h"

#define POS_ATTRIB 0
#define COLOR_ATTRIB 1
#define SCALEROT_ATTRIB 2
#define OFFSET_ATTRIB 3

struct Vertex {
  GLfloat pos[2];
  GLubyte rgba[4];
};
extern const Vertex QUAD[4];

// returns true if a GL error occurred
extern bool checkGlError(const char* funcName);
extern GLuint createShader(GLenum shaderType, const char* src);
extern GLuint createProgram(const char* vtxSrc, const char* fragSrc);

// ----------------------------------------------------------------------------
// Interface to the ES2 and ES3 renderers, used by JNI code.

class Renderer {
 public:
  ~Renderer();
  void resize(int w, int h);
  void render();
  void handleTouch(float x, float y);
  bool init();

  Renderer();

 protected:
  // return a pointer to a buffer of MAX_INSTANCES * sizeof(vec2).
  // the buffer is filled with per-instance offsets, then unmapped.
  float* mapOffsetBuf();
  void unmapOffsetBuf();
  // return a pointer to a buffer of MAX_INSTANCES * sizeof(vec4).
  // the buffer is filled with per-instance scale and rotation transforms.
  float* mapTransformBuf();
  void unmapTransformBuf();
  void draw(unsigned int numInstances);

 private:
  enum { VB_INSTANCE, VB_SCALEROT, VB_OFFSET, VB_COUNT };
  void calcSceneParams(unsigned int w, unsigned int h, float* offsets);
  void step();



  unsigned int mNumInstances;
  float mScale[2];
  float mAngularVelocity[MAX_INSTANCES];
  uint64_t mLastFrameNs;
  float mAngles[MAX_INSTANCES];

  float xpos = 0.0f;
  float ypos = 0.0f;

  float last_x_ = 1000.0f;
  float last_y_ = 1000.0f;
  bool first_touch_ = true;

  const EGLContext mEglContext;
  GLuint mProgram;
  GLuint mVB[VB_COUNT];
  GLuint mVBState;

};

extern Renderer* createES3Renderer();

#endif  // GLES3JNI_H
