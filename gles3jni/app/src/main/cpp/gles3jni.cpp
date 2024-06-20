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

#include <ctime>
#include <random>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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

static const char VERTEX_SHADER_TRIANGLE[] = R"glsl(
#version 320 es
precision mediump float;

layout(location = 0) in vec3 aPos;

uniform mat4 view;
uniform mat4 projection;
void main() {
    //gl_Position = vec4(aPos, 1.0);                     // 不做变换
    gl_Position = projection * view * vec4(aPos, 1.0); //做变换
}
)glsl";

static const char FRAGMENT_SHADER_TRIANGLE[] = R"glsl(
#version 320 es
precision mediump float;

out vec4 FragColor;

void main() {
    FragColor = vec4(1.0, 0.5, 0.2, 1.0);
}
)glsl";

static const char VERTEX_SHADER_CUBE[] = R"glsl(
#version 320 es
precision mediump float;

layout(location = 0) in vec3 aPos;

uniform mat4 view;
uniform mat4 projection;
void main() {
    gl_Position = projection * view * vec4(aPos, 1.0); //做变换
}
)glsl";

static const char FRAGMENT_SHADER_CUBE[] = R"glsl(
#version 320 es
precision mediump float;

out vec4 FragColor;

void main() {
    FragColor = vec4(1.0, 0.5, 0.2, 1.0);
}
)glsl";

static const char VERTEX_SHADER_INSTANCE[] = R"glsl(
#version 320 es
precision mediump float;

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aInstancePos; // 实例位置

uniform mat4 view;
uniform mat4 projection;
void main() {
    gl_Position = projection * view * vec4(aPos + aInstancePos, 1.0);
}
)glsl";

static const char FRAGMENT_SHADER_INSTANCE[] = R"glsl(
#version 320 es
precision mediump float;

out vec4 FragColor;

void main() {
    FragColor = vec4(1.0, 0.5, 0.2, 1.0);  // 橙色
}
)glsl";

static const char VERTEX_SHADER[] = R"glsl(
#version 320 es
precision mediump float;

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aInstancePos; // 实例位置

out vec3 FragPos;
out vec4 model_position;

uniform vec3 position;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 extrinsic_matrix;

void main() {
  FragPos = aPos + aInstancePos; // FragPos = aPos + position;
  gl_Position = projection * view * vec4(FragPos, 1.0);
  model_position = extrinsic_matrix * vec4(FragPos, 1.0);
}
)glsl";

static const char FRAGMENT_SHADER[] =
    R"glsl(
#version 320 es
precision mediump float;

in vec4 model_position;
out vec3 FragColor;

uniform sampler2D camera_texture;
uniform vec2 focal_lengths;
uniform vec2 cammera_principal_point;

void main() {
    float pz = model_position.z;
    if (abs(pz) < 0.00001) {
        pz = 0.1;
    }
    if (model_position.z < 0.0) {
        discard;
    }
    vec2 viewp = vec2(model_position.x/pz, model_position.y/pz);
    vec2 final_point = viewp * focal_lengths + cammera_principal_point;

    if (final_point.x < 0.0 ||  final_point.y < 0.0 || final_point.x > 1.0 || final_point.y > 1.0) {
           discard;
    }

    FragColor = texture(camera_texture, final_point).rgb;
}
)glsl";

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

GLuint loadTexture(const std::string& string_path) {
  const char* path = string_path.c_str();
  unsigned int textureID;
  glGenTextures(1, &textureID);

  int width, height, nrComponents;
  unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
  if (data) {
    GLenum format;
    if (nrComponents == 1)
      format = GL_RED;
    else if (nrComponents == 3)
      format = GL_RGB;
    else if (nrComponents == 4)
      format = GL_RGBA;

    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
                 GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
  } else {
    ALOGE("Texture failed to load at path: ");
    stbi_image_free(data);
  }

  return textureID;
}

std::vector<std::vector<int>> read_csv(const std::string& filename) {
  std::vector<std::vector<int>> data;

  // 打开文件
  std::ifstream infile(filename);
  std::string line;

  // 按行读取
  while (std::getline(infile, line)) {
    std::stringstream lineStream(line);
    std::string cell;
    std::vector<int> rowData;

    // 以逗号分隔每个字段
    while (getline(lineStream, cell, ',')) {
      // 将字符串转换为整数
      rowData.push_back(std::stoi(cell));
    }

    data.push_back(rowData);
  }

  return data;
}

/* 坐标变化以及调节基准 */
void GenCubePosition(const std::string& cordinate_path,
                     std::vector<glm::vec3>& cube_positions, glm::vec3 offset) {
  auto depth = 30;
  auto height = 100;
  auto width = 100;
  /* 加载 cube 坐标,  */
  for (int z = 2; z < 8; ++z) {
    const std::string filename = cordinate_path + std::to_string(z) + ".csv";
    std::vector<std::vector<int>> data = read_csv(filename);
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        /* 填充地面 */
        if (z == 2) {
          glm::vec3 temp_positon(x * 1.0f, y * 1.0f, z * 1.0f);
          cube_positions.push_back(temp_positon);
        }

        /* 添加边缘和立面 */
        if (y == 0 || y == 99 || x == 0 || x == 99) {
          glm::vec3 temp_positon(x * 1.0f, y * 1.0f, z * 1.0f);
          cube_positions.push_back(temp_positon);
        }

        /* 添加 voxels */
        if (data[y][x] != 17) {
          glm::vec3 temp_positon(x * 1.0f, y * 1.0f, z * 1.0f);
          cube_positions.push_back(temp_positon);
        }
      }
    }
  }

  std::vector<glm::vec3> wallPositions;
  // 生成四周墙壁
  for (int z = 0; z < depth; ++z) {
    for (int y = 0; y < height; ++y) {
      // 左右墙壁
      wallPositions.push_back(glm::vec3(0, y, z));
      wallPositions.push_back(glm::vec3(width - 1, y, z));
    }
    for (int x = 0; x < width; ++x) {
      // 前后墙壁
      wallPositions.push_back(glm::vec3(x, 0, z));
      wallPositions.push_back(glm::vec3(x, height - 1, z));
    }
  }
  cube_positions.insert(cube_positions.end(), wallPositions.begin(),
                        wallPositions.end());

  // 移除重复的顶点
  cube_positions.erase(
      std::unique(cube_positions.begin(), cube_positions.end()),
      cube_positions.end());

  /* cube 整体移动，以及转化到真实世界坐标 */
  for (auto& position : cube_positions) {
    position += offset;
  }
  for (auto& position : cube_positions) {
    position *= VOXEL_SIZE;
  }

  /* cube z 轴旋转 -90 度 */
  float rotationAngleDegrees = -90.0f;
  float rotationAngleRadians = glm::radians(rotationAngleDegrees);
  glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), rotationAngleRadians,
                                         glm::vec3(0.0f, 0.0f, 1.0f));
  for (auto& position : cube_positions) {
    position = glm::vec3(rotationMatrix * glm::vec4(position, 1.0f));
  }

  /* 对y轴反转 */
  for (auto& position : cube_positions) {
    position.y = -position.y;
  }
}
void GenerateModelMat(glm::quat& quaternion, glm::vec3& translationVector,
                      glm::mat4& model_mat, glm::vec3& t2_,
                      const glm::vec3& ExtrinsicOffset) {
  glm::quat rotation_final;
  {
    float angle_x_degrees = 0.0f;
    float angle_y_degrees = 0.0f;
    float angle_z_degrees = 90.0f;

    glm::quat rotation_x =
        glm::angleAxis(glm::radians(angle_x_degrees), glm::vec3(1, 0, 0));
    glm::quat rotation_y =
        glm::angleAxis(glm::radians(angle_y_degrees), glm::vec3(0, 1, 0));
    glm::quat rotation_z =
        glm::angleAxis(glm::radians(angle_z_degrees), glm::vec3(0, 0, 1));

    rotation_final = rotation_z * rotation_y * rotation_x;
  }

  quaternion = rotation_final * quaternion;
  translationVector += ExtrinsicOffset;
  glm::mat3 rotation_matrix_c2w = glm::mat3_cast(quaternion);
  t2_ = rotation_final * translationVector;

  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      model_mat[i][j] = rotation_matrix_c2w[i][j];
    }
  }
  model_mat[3][0] = t2_[0];
  model_mat[3][1] = t2_[1];
  model_mat[3][2] = t2_[2];
  model_mat[3][3] = 1.0f;
  model_mat = glm::inverse(model_mat);
};

// ----------------------------------------------------------------------------

Renderer::Renderer() : mEglContext(eglGetCurrentContext()), voxel_program_(0) {}

Renderer::~Renderer() {
  if (eglGetCurrentContext() != mEglContext) return;

  switch (DEBUG_MODE) {
    case 0:
      releaseVoxelResources();
      break;
    case 1:
      releaseTriangleResources();
      break;
    case 2:
      releaseCubeResources();
      break;
    case 3:
      releaseCubeResources();
      releaseInstanceResources();
      break;
  }
}

void Renderer::resize(int w, int h) {
  screen_x_ = w;
  screen_y_ = h;
  glViewport(0, 0, w, h);
}

void Renderer::render() {
  switch (DEBUG_MODE) {
    case 0:
      drawVoxels();
      break;
    case 1:
      drawTriangle();
      break;
    case 2:
      drawCube();
      break;
    case 3:
      drawInstance();
      break;
  }
  auto latest_frame = std::chrono::high_resolution_clock::now();
  auto frameDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
                           latest_frame - last_frame_)
                           .count();
  ALOGV("frame %lld", frameDuration);
  last_frame_ = latest_frame;
}

Renderer* createES3Renderer() {
  Renderer* renderer = new Renderer;


  switch (DEBUG_MODE) {
    case 0:
      if (!renderer->initVoxelResources()) {
        delete renderer;
        return NULL;
      }
      break;
    case 1:
      renderer->initTriangle();
      break;
    case 2:
      renderer->initCube();
      break;
    case 3:
      renderer->initCube();
      renderer->initInstance();
      break;
  }

  return renderer;
}
void GenRandomCubePositions(std::vector<glm::vec3>& cube_positions,
                            float numbers, float range) {
  // 随机数生成器初始化
  std::mt19937 gen(
      static_cast<unsigned int>(time(0)));  // 使用当前时间作为随机种子
  std::uniform_real_distribution<float> dis(
      -range, range);  // 定义一个分布，在-range到range之间

  for (int i = 0; i < numbers; ++i) {
    // 生成随机x, y, z坐标
    float x = dis(gen);
    float y = dis(gen);
    float z = dis(gen);
    // 用生成的坐标创建一个vec3，并添加到vector中
    cube_positions.emplace_back(x, y, z);
  }
}

/* 可以加读写锁或者用原子数保护，当前省时间没有进行此操作 */
void Renderer::handleTouch(float x, float y) {
  delta_x = x;
  delta_y = y;
}

bool Renderer::initVoxelResources() {
  auto last_frame_ = std::chrono::high_resolution_clock::now();

  /* 生成缩放的顶点数据 */

  float scaled_vertices[sizeof(CUBE_VERTICES) / sizeof(float)];
  for (size_t i = 0; i < sizeof(CUBE_VERTICES) / sizeof(float); i += 3) {
    scaled_vertices[i] = CUBE_VERTICES[i] * VOXEL_SIZE;          // x坐标
    scaled_vertices[i + 1] = CUBE_VERTICES[i + 1] * VOXEL_SIZE;  // y坐标
    scaled_vertices[i + 2] = CUBE_VERTICES[i + 2] * VOXEL_SIZE;  // z坐标
  }

  voxel_program_ = createProgram(VERTEX_SHADER, FRAGMENT_SHADER);
  checkGlError("create program");

  glGenVertexArrays(1, &cube_vao_);
  glGenBuffers(1, &cube_vbo_);

  glBindBuffer(GL_ARRAY_BUFFER, cube_vbo_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(scaled_vertices), scaled_vertices,
               GL_STATIC_DRAW);

  glBindVertexArray(cube_vao_);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  checkGlError("init Voxel vao vbo");

  /* 生成立方体 */
  GenCubePosition(VOXEL_COORDINATE_PATH, cube_positions_, VOTEX_OFFSET);

  glGenBuffers(1, &voxel_instance_vbo_);
  glBindBuffer(GL_ARRAY_BUFFER, voxel_instance_vbo_);
  glBufferData(GL_ARRAY_BUFFER, cube_positions_.size() * sizeof(glm::vec3),
               &cube_positions_[0], GL_STATIC_DRAW);
  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, voxel_instance_vbo_);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glVertexAttribDivisor(1, 1);
  checkGlError("init voxel_instance_vbo_");

  LoadTextures();

  /* 生成内外参 */
  for (auto i = 0; i < 6; i++) {
    GenerateModelMat(quaternions_[i], translation_vectors_[i], model_mat_[i],
                     t2_[i], ExtrinsicOffset);
  }

  gl_camera_.Position = t2_[0]; /* 相机放到前摄位置 */

  glUseProgram(voxel_program_);

  checkGlError("use program");
  glUniform1i(glGetUniformLocation(voxel_program_, "camera_texture"), 0);

  glm::vec2 focal_length = glm::vec2(intrinsics_[0][0][0] / IMAGE_WIDTH,
                                     intrinsics_[0][1][1] / IMAGE_HEIGHT);
  glm::vec2 principal_point = glm::vec2(intrinsics_[0][0][2] / IMAGE_WIDTH,
                                        intrinsics_[0][1][2] / IMAGE_HEIGHT);
  glUniform2f(glGetUniformLocation(voxel_program_, "focal_lengths"),
              focal_length.x, focal_length.y);
  glUniform2f(glGetUniformLocation(voxel_program_, "cammera_principal_point"),
              principal_point.x, principal_point.y);
  glUseProgram(0);

  return true;
}
void Renderer::drawVoxels() {
  checkGlError("began draw");

  gl_camera_.ProcessMouseMovement(delta_x, delta_y);

  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  checkGlError("clear");

  glUseProgram(voxel_program_);
//  glEnable(GL_CULL_FACE);
  glCullFace(GL_CCW);

  glm::mat4 projection =
      glm::perspective(glm::radians(gl_camera_.Zoom),
                       (float)screen_x_ / (float)screen_y_, 0.1f, 100.0f);
  glm::mat4 view = gl_camera_.GetViewMatrix();
  glUniformMatrix4fv(glGetUniformLocation(voxel_program_, "projection"), 1,
                     GL_FALSE, glm::value_ptr(projection));
  glUniformMatrix4fv(glGetUniformLocation(voxel_program_, "view"), 1, GL_FALSE,
                     glm::value_ptr(view));
  glUniform1i(glGetUniformLocation(voxel_program_, "camera_texture"), 0);
  glActiveTexture(GL_TEXTURE0);
  glBindVertexArray(cube_vao_);

  for (auto i = 0; i < CAMERA_COUNTS; i++) {
    glBindTexture(GL_TEXTURE_2D, camera_textures[i]);
    glUniformMatrix4fv(glGetUniformLocation(voxel_program_, "extrinsic_matrix"),
                       1, GL_FALSE, glm::value_ptr(model_mat_[i]));
    glDrawArraysInstanced(GL_TRIANGLES, 0, 36, cube_positions_.size());
  }
  checkGlError("draw finished");
}

void Renderer::LoadTextures() {
  camera_textures[0] = loadTexture(cam_front_path);
  camera_textures[1] = loadTexture(cam_back_path);
  camera_textures[2] = loadTexture(cam_front_left_path);
  camera_textures[3] = loadTexture(cam_front_right_path);
  camera_textures[4] = loadTexture(cam_back_left_path);
  camera_textures[5] = loadTexture(cam_back_right_path);

  for (auto i = 0; i < CAMERA_COUNTS; i++) {
    ALOGI("camera texture %d = %d", i, camera_textures[i]);
  }
  checkGlError("LoadTextures");
}
void Renderer::initTriangle() {
  triangle_program_ =
      createProgram(VERTEX_SHADER_TRIANGLE, FRAGMENT_SHADER_TRIANGLE);
  glGenVertexArrays(1, &triangle_vao_);
  glGenBuffers(1, &triangle_vbo_);

  glBindVertexArray(triangle_vao_);
  glBindBuffer(GL_ARRAY_BUFFER, triangle_vbo_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(triangleVertices), triangleVertices,
               GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  checkGlError("init triangle");
}
void Renderer::drawTriangle() {
  updatingCameraParams();

  // todo 原先的方案在触控屏上有问题，这个应该研究下原因
  //  projection =glm::perspective(glm::radians(gl_camera_.Zoom),
  //                       (float)screen_x_ / (float)screen_y_, 0.1f, 100.0f);
  //  view = gl_camera_.GetViewMatrix();

  // 绘制部分
  glClearColor(0.5f, 0.5f, 0.1f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glUseProgram(triangle_program_);

  // set camera
  unsigned int viewLoc = glGetUniformLocation(triangle_program_, "view");
  unsigned int projLoc = glGetUniformLocation(triangle_program_, "projection");
  glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view_));
  glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection_));

  // draw
  glBindVertexArray(triangle_vao_);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  checkGlError("draw triangle finished");
}
void Renderer::releaseVoxelResources() {
  for (auto i = 0; i < CAMERA_COUNTS; i++) {
    if (camera_textures[i]) {
      glDeleteTextures(1, &camera_textures[i]);
      camera_textures[i] = 0;
    }
  }

  glDeleteVertexArrays(1, &cube_vao_);
  glDeleteBuffers(1, &cube_vbo_);
  glDeleteBuffers(1, &voxel_instance_vbo_);
  glDeleteProgram(voxel_program_);
}
void Renderer::releaseTriangleResources() {
  glDeleteVertexArrays(1, &triangle_vao_);
  glDeleteBuffers(1, &triangle_vbo_);
  glDeleteProgram(triangle_program_);
}
void Renderer::releaseCubeResources() {
  glDeleteVertexArrays(1, &cube_vao_);
  glDeleteBuffers(1, &cube_vbo_);
  glDeleteProgram(cube_program_);
}
void Renderer::initCube() {
  float scaled_vertices[sizeof(CUBE_VERTICES) / sizeof(float)];
  for (size_t i = 0; i < sizeof(CUBE_VERTICES) / sizeof(float); i += 3) {
    scaled_vertices[i] = CUBE_VERTICES[i] * VOXEL_SIZE;          // x坐标
    scaled_vertices[i + 1] = CUBE_VERTICES[i + 1] * VOXEL_SIZE;  // y坐标
    scaled_vertices[i + 2] = CUBE_VERTICES[i + 2] * VOXEL_SIZE;  // z坐标
  }

  cube_program_ = createProgram(VERTEX_SHADER_CUBE, FRAGMENT_SHADER_CUBE);

  glGenVertexArrays(1, &cube_vao_);
  glGenBuffers(1, &cube_vbo_);

  glBindBuffer(GL_ARRAY_BUFFER, cube_vbo_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(scaled_vertices), scaled_vertices,
               GL_STATIC_DRAW);

  glBindVertexArray(cube_vao_);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
  glEnableVertexAttribArray(0);
  checkGlError("init cube vao vbo");
}
void Renderer::drawCube() {
  updatingCameraParams();

  // 绘制部分
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glUseProgram(cube_program_);

  // set camera
  unsigned int viewLoc = glGetUniformLocation(cube_program_, "view");
  unsigned int projLoc = glGetUniformLocation(cube_program_, "projection");
  glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view_));
  glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection_));

  // draw
  glBindVertexArray(cube_vao_);
  glDrawArrays(GL_TRIANGLES, 0, 36);
  checkGlError("draw cube");
}
void Renderer::initInstance() {
  instance_program_ =
      createProgram(VERTEX_SHADER_INSTANCE, FRAGMENT_SHADER_INSTANCE);

  //  GenRandomCubePositions(cube_positions_, INSTANCE_NUMBERS, RANGE); //
  //  随机位置
  GenCubePosition(VOXEL_COORDINATE_PATH, cube_positions_,
                  VOTEX_OFFSET);  // 测试加载，也顺利

  glGenBuffers(1, &voxel_instance_vbo_);
  glBindBuffer(GL_ARRAY_BUFFER, voxel_instance_vbo_);
  glBufferData(GL_ARRAY_BUFFER, cube_positions_.size() * sizeof(glm::vec3),
               &cube_positions_[0], GL_STATIC_DRAW);
  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, voxel_instance_vbo_);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glVertexAttribDivisor(1, 1);
  checkGlError("init voxel_instance_vbo_");
}
void Renderer::releaseInstanceResources() {
  glDeleteBuffers(1, &voxel_instance_vbo_);
  glDeleteProgram(instance_program_);
}
void Renderer::drawInstance() {
  updatingCameraParams();

  // 绘制部分
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glUseProgram(instance_program_);

  // set camera
  unsigned int viewLoc = glGetUniformLocation(instance_program_, "view");
  unsigned int projLoc = glGetUniformLocation(instance_program_, "projection");
  glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view_));
  glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection_));

  // draw
  glBindVertexArray(cube_vao_);
  glDrawArraysInstanced(GL_TRIANGLES, 0, 36, cube_positions_.size());
}
void Renderer::updatingCameraParams() {
  const auto MouseSensitivity = 0.001;
  // 摄像机的方向变量
  float xoffset = -1.0 * delta_x * MouseSensitivity;
  float yoffset = delta_y * MouseSensitivity;

  // 更新摄像机的Yaw和Pitch值
  Yaw_ += xoffset;
  Pitch_ += yoffset;

  // 限制Pitch，防止出现奇怪的循环视图
  const float maxPitch = 89.0f;
  const float minPitch = -89.0f;
  Pitch_ = std::max(minPitch, std::min(Pitch_, maxPitch));

  // camera 计算方法
  glm::vec3 front;
  front.x = cos(glm::radians(Yaw_)) * cos(glm::radians(Pitch_));
  front.y = sin(glm::radians(Pitch_));
  front.z = sin(glm::radians(Yaw_)) * cos(glm::radians(Pitch_));
  glm::vec3 cameraFront = glm::normalize(front);

  glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);  // 上方为+Y轴
  view_ =
      glm::lookAt(camera_position_, camera_position_ + cameraFront, cameraUp);
  projection_ = glm::perspective(
      glm::radians(45.0f), (float)screen_x_ / (float)screen_y_, 0.1f, 100.0f);
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
JNIEXPORT void JNICALL Java_com_android_gles3jni_GLES3JNILib_onTouchMove(
    JNIEnv* env, jobject obj, jfloat delta_x, jfloat delta_y);
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

JNIEXPORT void JNICALL Java_com_android_gles3jni_GLES3JNILib_onTouchMove(
    JNIEnv* env, jobject obj, jfloat delta_x, jfloat delta_y) {
  // 简单的跨线程，
  float x = static_cast<float>(delta_x);
  float y = static_cast<float>(delta_y);
  if (g_renderer) {
    g_renderer->handleTouch(x, y);
  }
}
