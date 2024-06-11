//
// Created by 王锐超 on 2024/6/11.
//

#ifndef NDK_SAMPLES_COMPUTESHADERPARTICLES_H
#define NDK_SAMPLES_COMPUTESHADERPARTICLES_H

#include <GLES3/gl32.h>
#include <GLES2/gl2ext.h>
#include <android/log.h>

#define LOG_TAG "Test_CS_Particles_"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

class ComputeShaderParticles {
 public:
  ComputeShaderParticles();
  ~ComputeShaderParticles();
  void renderFrame();
  bool setupGraphics(int w, int h);

  private:
  static void checkGlError(const char* op);
  static void printGLString(const char* name, GLenum s);
  GLuint loadShader(GLenum shaderType, const char* pSource);
  GLuint createProgram(const char* pVertexSource, const char* pFragmentSource);

  GLuint gProgram;
  GLuint gvPositionHandle;
  int window_width;
  int window_height;
};

#endif  // NDK_SAMPLES_COMPUTESHADERPARTICLES_H
