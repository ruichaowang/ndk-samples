//
// Created by 王锐超 on 2024/6/11.
// 使用计算着色器实现粒子的物理效果
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

  static constexpr int PARTICLES_COUNT_X = 256;   // shader 中要和这个一致
  static constexpr int PARTICLES_COUNT_Y = 256;
  static constexpr int DISPLAY_X = 1024;
  static constexpr int DISPLAY_Y = 1024;
  static constexpr float massMin = 0.75;
  static constexpr float massMax = 1.25;

  struct ParticlesBuffer {
    float particles_position_x[PARTICLES_COUNT_X][PARTICLES_COUNT_Y];
    float particles_position_y[PARTICLES_COUNT_X][PARTICLES_COUNT_Y];
    float particles_velocity_x[PARTICLES_COUNT_X][PARTICLES_COUNT_Y];
    float particles_velocity_y[PARTICLES_COUNT_X][PARTICLES_COUNT_Y];
    float particles_mass[PARTICLES_COUNT_X][PARTICLES_COUNT_Y];
  };

  struct ParticlesDisplayBuffer {
    int particles_count[DISPLAY_X][DISPLAY_Y]; // 真正用来显示的每一个点的粒子数，这个是图像的分辨率
  };

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
  GLuint createComputeShaderProgram(const char* pComputeSource);
  void createSSBO();
  void initSSBOParticlesData();
  void initSSBODisplayData();
  void releaseSSBO();
  void genVertexBuffers();

  int window_width;
  int window_height;
  GLuint renderProgramID;                        // Create the gl render program
  GLuint updateParticlesProgramID;                    // compute shader program
  GLuint ssbo_particles_ = 0;
  GLuint ssbo_display_ = 0;
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
