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

#include <EGL/egl.h>
#include <android/log.h>
#include <math.h>

#include <array>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <sstream>
#include <vector>

#include "camera.h"
#include "gles3jni.h"

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
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
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

// 由于当前绘制不出来东西， 先画一个三角形确认原因

const GLfloat triangleVertices[] = {
    0.0f,  1.0f,  0.0f,  // 顶点
    -1.0f, -1.0f, 0.0f,  // 左下角
    1.0f,  -1.0f, 0.0f   // 右下角
};

const GLfloat CUBE_VERTICES[] = {
    // positions
    -0.5f, -0.5f, -0.5f, 0.5f,  -0.5f, -0.5f, 0.5f,  0.5f,  -0.5f,
    0.5f,  0.5f,  -0.5f, -0.5f, 0.5f,  -0.5f, -0.5f, -0.5f, -0.5f,

    -0.5f, -0.5f, 0.5f,  0.5f,  -0.5f, 0.5f,  0.5f,  0.5f,  0.5f,
    0.5f,  0.5f,  0.5f,  -0.5f, 0.5f,  0.5f,  -0.5f, -0.5f, 0.5f,

    -0.5f, 0.5f,  0.5f,  -0.5f, 0.5f,  -0.5f, -0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, 0.5f,  -0.5f, 0.5f,  0.5f,

    0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  -0.5f, 0.5f,  -0.5f, -0.5f,
    0.5f,  -0.5f, -0.5f, 0.5f,  -0.5f, 0.5f,  0.5f,  0.5f,  0.5f,

    -0.5f, -0.5f, -0.5f, 0.5f,  -0.5f, -0.5f, 0.5f,  -0.5f, 0.5f,
    0.5f,  -0.5f, 0.5f,  -0.5f, -0.5f, 0.5f,  -0.5f, -0.5f, -0.5f,

    -0.5f, 0.5f,  -0.5f, 0.5f,  0.5f,  -0.5f, 0.5f,  0.5f,  0.5f,
    0.5f,  0.5f,  0.5f,  -0.5f, 0.5f,  0.5f,  -0.5f, 0.5f,  -0.5f};

/* 把车挪到整个模型的中心,调节地面的基准 -2 是推测值 */
const auto VOTEX_OFFSET = glm::vec3(-50.5f, -50.5f, -2.0f);  //?-35
/* 外参的坐标系和车辆坐标系的变化，这个数为推测出来的 */
const auto ExtrinsicOffset = glm::vec3(-0.0, 1.0, 1.5);
const auto debug_draw_pano = true;
const int debug_discard = 1;
const auto IMAGE_WIDTH = 1600.0;
const auto IMAGE_HEIGHT = 900.0;
const float VOXEL_SIZE = 1.024f;
const int CAMERA_COUNTS = 6;
const std::string CACHE_PATH = "/data/data/com.android.gles3jni/cache";
const std::string cam_front_path =
    CACHE_PATH +
    "/camera/"
    "n015-2018-10-08-15-36-50+0800__CAM_FRONT__1538984245412460.jpg";
const auto cam_back_path =
    CACHE_PATH +
    "/camera/"
    "n015-2018-10-08-15-36-50+0800__CAM_BACK__1538984245437525.jpg";
const auto cam_front_left_path =
    CACHE_PATH +
    "/camera/"
    "n015-2018-10-08-15-36-50+0800__CAM_FRONT_LEFT__1538984245404844.jpg";
const auto cam_front_right_path =
    CACHE_PATH +
    "/camera/"
    "n015-2018-10-08-15-36-50+0800__CAM_FRONT_RIGHT__1538984245420339.jpg";
const auto cam_back_left_path =
    CACHE_PATH +
    "/camera/"
    "n015-2018-10-08-15-36-50+0800__CAM_BACK_LEFT__1538984245447423.jpg";
const auto cam_back_right_path =
    CACHE_PATH +
    "/camera/"
    "n015-2018-10-08-15-36-50+0800__CAM_BACK_RIGHT__1538984245427893.jpg";
const auto VOXEL_COORDINATE_PATH =
    "/data/data/com.android.gles3jni/cache/cordinate/slice_";

const auto intrinsics_front =
    glm::mat3(1266.417203046554, 0.0, 816.2670197447984, 0.0, 1266.417203046554,
              491.50706579294757, 0.0, 0.0, 1.0);
const auto quaternion_front =
    glm::quat(0.4998015430569128, -0.5030316162024876, 0.4997798114386805,
              -0.49737083824542755);
const auto translation_vectors_front =
    glm::vec3(1.70079118954, 0.0159456324149, 1.51095763913);

const auto intrinsics_rear =
    glm::mat3(809.2209905677063, 0.0, 829.2196003259838, 0.0, 809.2209905677063,
              481.77842384512485, 0.0, 0.0, 1.0);
const auto quaternion_rear = glm::quat(0.5037872666382278, -0.49740249788611096,
                                       -0.4941850223835201, 0.5045496097725578);
const auto translation_vectors_rear =
    glm::vec3(0.0283260309358, 0.00345136761476, 1.57910346144);

const auto intrinsics_front_left =
    glm::mat3(1272.5979470598488, 0.0, 826.6154927353808, 0.0,
              1272.5979470598488, 479.75165386361925, 0.0, 0.0, 1.0);
const auto quaternion_front_left =
    glm::quat(0.6757265034669446, -0.6736266522251881, 0.21214015046209478,
              -0.21122827103904068);
const auto translation_vectors_front_left =
    glm::vec3(1.52387798135, 0.494631336551, 1.50932822144);

const auto intrinsics_front_right =
    glm::mat3(1260.8474446004698, 0.0, 807.968244525554, 0.0,
              1260.8474446004698, 495.3344268742088, 0.0, 0.0, 1.0);
const auto quaternion_front_right =
    glm::quat(0.2060347966337182, -0.2026940577919598, 0.6824507824531167,
              -0.6713610884174485);
const auto translation_vectors_front_right =
    glm::vec3(1.5508477543, -0.493404796419, 1.49574800619);

const auto intrinsics_back_left =
    glm::mat3(1256.7414812095406, 0.0, 792.1125740759628, 0.0,
              1256.7414812095406, 492.7757465151356, 0.0, 0.0, 1.0);
const auto quaternion_back_left =
    glm::quat(0.6924185592174665, -0.7031619420114925, -0.11648342771943819,
              0.11203317912370753);
const auto translation_vectors_back_left =
    glm::vec3(1.0148780988, -0.480568219723, 1.56239545128);

const auto intrinsics_back_right =
    glm::mat3(1259.5137405846733, 0.0, 807.2529053838625, 0.0,
              1259.5137405846733, 501.19579884916527, 0.0, 0.0, 1.0);
const auto quaternion_back_right =
    glm::quat(0.12280980120078765, -0.132400842670559, -0.7004305821388234,
              0.690496031265798);
const auto translation_vectors_back_right =
    glm::vec3(1.0148780988, -0.480568219723, 1.56239545128);

const auto DEBUG_MODE = 2; // 0 voxels, 1 triangle, 2 cube,

// returns true if a GL error occurred
bool checkGlError(const char *funcName);
GLuint createShader(GLenum shaderType, const char *src);
GLuint createProgram(const char *vtxSrc, const char *fragSrc);
GLuint loadTexture(const std::string &string_path);
std::vector<std::vector<int>> read_csv(const std::string &filename);
void GenCubePosition(const std::string &cordinate_path,
                     std::vector<glm::vec3> &cube_positions, glm::vec3 offset);
void GenerateModelMat(glm::quat &quaternion, glm::vec3 &translationVector,
                      glm::mat4 &model_mat, glm::vec3 &t2_,
                      const glm::vec3 &ExtrinsicOffset);
// ----------------------------------------------------------------------------
// Interface to the ES2 and ES3 renderers, used by JNI code.

class Renderer {
 public:
  ~Renderer();
  void resize(int w, int h);
  void render();
  void handleTouch(float x, float y);
  bool initVoxelResources();
  void initTriangle();
  void initCube();
  Renderer();

 private:
  void step();
  void LoadTextures();
  void drawTriangle();
  void drawCube();
  void drawVoxels();
  void releaseVoxelResources();
  void releaseTriangleResources();
  void releaseCubeResources();

  float xpos = 1000.0f;
  float ypos = 1000.0f;
  float last_x_ = 1000.0f;
  float last_y_ = 1000.0f;
  int screen_x_ = 0;
  int screen_y_ = 0;

  const EGLContext mEglContext;
  GLuint voxel_program_, cube_vbo_, cube_vao_, voxel_instance_vbo_;
  GLuint triangle_program_, triangle_vbo_, triangle_vao_;
  GLuint cube_program_;

  std::array<GLuint, CAMERA_COUNTS> camera_textures;
  std::array<glm::mat3, CAMERA_COUNTS> intrinsics_ = {
      intrinsics_front,       intrinsics_rear,      intrinsics_front_left,
      intrinsics_front_right, intrinsics_back_left, intrinsics_back_right};
  std::array<glm::quat, CAMERA_COUNTS> quaternions_{
      quaternion_front,       quaternion_rear,      quaternion_front_left,
      quaternion_front_right, quaternion_back_left, quaternion_back_right};
  std::array<glm::vec3, CAMERA_COUNTS> translation_vectors_ = {
      translation_vectors_front,      translation_vectors_rear,
      translation_vectors_front_left, translation_vectors_front_right,
      translation_vectors_back_left,  translation_vectors_back_right};
  std::array<glm::vec3, CAMERA_COUNTS> t2_;
  std::array<glm::mat4, CAMERA_COUNTS> model_mat_;
  /* 定义立方体的位置 */
  std::vector<glm::vec3> cube_positions_ = {};
  Camera gl_camera_;
  float Yaw_ = -90.0f; // Yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
  float Pitch_ = 0.0f; // 初始俯仰角为0
};

extern Renderer *createES3Renderer();

#endif  // GLES3JNI_H
