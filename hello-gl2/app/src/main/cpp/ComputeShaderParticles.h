//
// Created by 王锐超 on 2024/6/11.
// 当前所有image read/load 不支持 rgba32f
//

#ifndef NDK_SAMPLES_COMPUTESHADERPARTICLES_H
#define NDK_SAMPLES_COMPUTESHADERPARTICLES_H

#include <GLES3/gl32.h>
#include <android/log.h>
#include <iostream>
#define LOG_TAG "Test_CS_Particles_"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

class ComputeShaderParticles {
  struct ParticlesBuffer {
    float particles_position_x[1024][1024];
    float particles_position_y[1024][1024];
    float particles_velocity_x[1024][1024];
    float particles_velocity_y[1024][1024];
    float particles_mass[1024][1024];
    int particles_count[1024][1024];                           // 每一个点的粒子数
  };

 public:
  ComputeShaderParticles();
  ~ComputeShaderParticles();
  void renderFrame();
  bool setupGraphics(int w, int h);

 private:
  static void checkGlError(const char* op);
  static void printGLString(const char* name, GLenum s);
  static std::string formatShaderCode(std::string shaderCode);
  GLuint loadShader(GLenum shaderType, const char* pSource);
  GLuint createProgram(const char* pVertexSource, const char* pFragmentSource);
  GLuint createComputeShaderProgram(const char* pComputeSource);
  void createSSBO();
  void initParticleProperties();
  void releaseSSBO();
  void genVertexBuffers();

  int window_width;
  int window_height;
  GLuint renderProgramID;                        // Create the gl render program
  GLuint updateParticlesProgramID;                    // compute shader program
  GLuint ssbo_particles_ = 0;
  std::atomic <bool> isSSBOReady = false;
  GLuint vertArray;   //这个最后没有清理
  GLuint posBuf;     //这个还没有清理
  double lastFrameTime = 0.0;
  double fpsCountLastTime = 0.0;
  double lastUpdateTime = 0.0;
  float m = 3.0;
  float n = 5.0;
};

#endif  // NDK_SAMPLES_COMPUTESHADERPARTICLES_H