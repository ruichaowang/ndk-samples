//
// Created by 王锐超 on 2024/6/11.
//

#include "ComputeShaderParticles.h"

#include <GLES2/gl2ext.h>
#include <GLES3/gl32.h>
#include <android/log.h>
#include <jni.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#include <iostream>

static double getCurrentTime() {
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  return now.tv_sec + now.tv_nsec / 1000000000.0;
}

const auto vertexShaderCode = R"glsl(
#version 320 es
precision mediump float;
out vec2 uv;
in vec2 pos;

void main() {
  gl_Position = vec4(pos.x, pos.y, 0.0, 1.0);
  uv = 0.5 * pos + 0.5;
}
)glsl";

const auto fragmentShaderCode = R"glsl(
#version 320 es
precision mediump float;
#define DISPLAY_X 1440
#define DISPLAY_Y 1440
in vec2 uv;
out vec4 outColor;

layout(std430, binding = 1) buffer SSBO_display {
  int particles_count[DISPLAY_X][DISPLAY_Y];
};

void main() {
  int x = int(uv.x * float(DISPLAY_X));
  int y = int(uv.y * float(DISPLAY_Y));
  int count = particles_count[x][y];
  float v =  float(count) / 2.0;       //这个数据可以改
  particles_count[x][y] = 0;
  outColor = vec4(v * 0.7 + 0.0, v * 0.7 + 0.0, v * 0.9 + 0.0, 1.0);  //填充颜色,参数按照固定值去设定
}
)glsl";

static auto updateParticlesComputeShaderCode = R"glsl(
#version 320 es
precision highp float;
precision highp int;
#define PARTICLES_COUNT_X 512
#define PARTICLES_COUNT_Y 512
#define DISPLAY_X 1440
#define DISPLAY_Y 1440

layout(local_size_x = 16, local_size_y = 16) in;

layout(std140, binding = 0) buffer SSBO_particles {
  vec4 particles_properties[PARTICLES_COUNT_X][PARTICLES_COUNT_Y];
  vec4 particles_mass[PARTICLES_COUNT_X][PARTICLES_COUNT_Y];
};

layout(std430, binding = 1) buffer SSBO_display {
  int particles_count[DISPLAY_X][DISPLAY_Y];
};

uniform float timeSinceLastFrame;
uniform float m;
uniform float n;
float w = float(DISPLAY_X), h = float(DISPLAY_Y);
float PI = 3.14159265359;

float rand(vec2 co) {
	float a = 12.9898;
	float b = 78.233;
	float c = 43758.5453;
	float dt = dot(co.xy ,vec2(a, b));
	float sn = mod(dt, PI);
	return fract(sin(sn) * c);
}

float chladni_equation(ivec2 id) {
	float scaledx = float(id.x) / w - 0.5;
	float scaledy = 0.5 - float(id.y) / h;
	float chladni = (cos(n * PI * scaledx) * cos(m * PI * scaledy) - cos(m * PI * scaledx) * cos(n * PI * scaledy)) / 2.0;
	return abs(chladni);
}

ivec2 genGradients(ivec2 id) {
	float min_value = chladni_equation(id);
	ivec2 rets[8];
	int can_use = 0;

	for (int i = 0; i < 8; i++) {
		rets[uint(i)] = ivec2(0, 0);
	}

	for (int i = -1; i <= 1; i++) {
		for (int j = -1; j <= 1; j++) {
			if (i == 0 && j == 0) {
				continue;
			}
			if (id.x + i < 0 || id.x + i >= int(w) || id.y + j < 0 || id.y + j >= int(h)) {
				continue;
			}
			ivec2 id_offset = ivec2(id.x + i, id.y + j);
			float value = chladni_equation(id_offset);
			if (value <= min_value) {
				if (value < min_value) {
					min_value = value;
					for (int k = 0; k < 8; k++) {
						rets[uint(k)] = ivec2(0, 0);
					}
					can_use = 0;
				}

				rets[uint(can_use)] = ivec2(i, j);
                                can_use += 1;
			}
		}
	}

	int choice = int(floor(rand(vec2( float(id.x) / w, float(id.y) / h)) * float(can_use)));
	return rets[choice];
}

void main() {
	ivec2 id = ivec2(gl_GlobalInvocationID.xy);
        vec4 v = particles_properties[id.x][id.y];
        float mass = particles_mass[id.x][id.y].x;

	// 更新位置
//	v.x += v.z * timeSinceLastFrame;
//	v.y += v.w * timeSinceLastFrame;

	// 力的方向
	ivec2 gradient = genGradients(ivec2(int(v.x), int(v.y)));
	// 力的大小
	float vibration = chladni_equation(ivec2(int(v.x), int(v.y)));
	// 求出加速度
	float c = (timeSinceLastFrame * (vibration * 25.0 * 20.0)) / mass;

        // 更新位置
        v.x += v.z * timeSinceLastFrame * (vibration + 0.2);
        v.y += v.w * timeSinceLastFrame * (vibration + 0.2);
	// 更新下一帧速度
	v.z += c * float(gradient.x);
	v.w += c * float(gradient.y);

        float x = v.x;
	float y = v.y;
        float rand = sin(float(particles_count[int(v.x)][int(v.y)]));
	if ((x - y < 3.0 && x - y > -3.0) || (x + y < w + 3.0 && x + y > w - 3.0)) {
		if (rand(vec2(x, y)) < 0.2) {
                float x_amend = rand(vec2(0.1 + timeSinceLastFrame, y + rand + timeSinceLastFrame)) * w;
                float y_amend = rand(vec2(x + rand + timeSinceLastFrame, 0.4 + timeSinceLastFrame)) * h;
                v.x = x_amend;
                v.y = y_amend;
		}
	}

        if ((x < 3.0 || x > w - 3.0) || (y < 3.0 || y > h - 3.0)) {
            if (rand(vec2(x, y)) < 0.1) {
                float x_amend = rand(vec2(0.1 + timeSinceLastFrame, y + rand + timeSinceLastFrame)) * w;
                float y_amend = rand(vec2(x + rand + timeSinceLastFrame, 0.4 + timeSinceLastFrame)) * h;
                v.x = x_amend;
                v.y = y_amend;
            }
        }

        int count = particles_count[int(v.x)][int(v.y)];
        atomicAdd(particles_count[int(v.x)][int(v.y)], 1);

        float drag1 = 0.8;
        float drag2 = 0.0001 * float(v.z*v.z + v.w*v.w);
        float drag3 = 0.3 * float(count);
        float drag = timeSinceLastFrame * (drag1 + drag2 + drag3);
        if (v.z > drag) v.z -= drag;
        else if (v.z < -drag) v.z += drag;
        else v.z = 0.0;
        if (v.w > drag) v.w -= drag;
        else if (v.w < -drag) v.w += drag;
        else v.w = 0.0;

        float s = 0.5;
	if (v.x < 0.0) { v.x = 0.0; v.z *= -s; v.w *= s; }
	if (v.x > w -1.0) { v.x = w -1.0; v.z *= -s; v.w *= s; }
	if (v.y < 0.0) { v.y = 0.0; v.w *= -s; v.z *= s; }
	if (v.y > h -1.0) { v.y = h -1.0; v.w *= -s; v.z *= s; }

        // 保存结果到 SSBO
        particles_properties[id.x][id.y] = v;

}
)glsl";

const GLfloat gTriangleVertices[] = {0.0f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f};

ComputeShaderParticles::ComputeShaderParticles() {}

ComputeShaderParticles::~ComputeShaderParticles() { releaseSSBO(); }

void ComputeShaderParticles::checkGlError(const char *op) {
  for (GLint error = glGetError(); error; error = glGetError()) {
    LOGI("after %s() glError (0x%x)\n", op, error);
  }
}
void ComputeShaderParticles::printGLString(const char *name, GLenum s) {
  const char *v = (const char *)glGetString(s);
  LOGI("GL %s = %s\n", name, v);
}
GLuint ComputeShaderParticles::loadShader(GLenum shaderType,
                                          const char *pSource) {
  GLuint shader = glCreateShader(shaderType);
  if (shader) {
    glShaderSource(shader, 1, &pSource, NULL);
    glCompileShader(shader);
    GLint compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
      GLint infoLen = 0;
      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
      if (infoLen) {
        char *buf = (char *)malloc(infoLen);
        if (buf) {
          glGetShaderInfoLog(shader, infoLen, NULL, buf);
          LOGE("Could not compile shader %d:\n%s\n", shaderType, buf);
          free(buf);
        }
        glDeleteShader(shader);
        shader = 0;
      }
    }
  }
  return shader;
}
GLuint ComputeShaderParticles::createProgram(const char *pVertexSource,
                                             const char *pFragmentSource) {
  GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
  if (!vertexShader) {
    return 0;
  }

  GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
  if (!pixelShader) {
    return 0;
  }

  GLuint program = glCreateProgram();
  if (program) {
    glAttachShader(program, vertexShader);
    checkGlError("glAttachShader");
    glAttachShader(program, pixelShader);
    checkGlError("glAttachShader");
    glLinkProgram(program);
    GLint linkStatus = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if (linkStatus != GL_TRUE) {
      GLint bufLength = 0;
      glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
      if (bufLength) {
        char *buf = (char *)malloc(bufLength);
        if (buf) {
          glGetProgramInfoLog(program, bufLength, NULL, buf);
          LOGE("Could not link program:\n%s\n", buf);
          free(buf);
        }
      }
      glDeleteProgram(program);
      program = 0;
    }
  }
  return program;
}
void ComputeShaderParticles::renderFrame() {
  glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
  checkGlError("glClear");

  if (!isSSBOReady) {
    return;
  }

  double currentTime = getCurrentTime();
  double timeSinceLastFrame = currentTime - lastFrameTime;
  lastFrameTime += timeSinceLastFrame;
  if (currentTime - lastUpdateTime > 1.0) {
    lastUpdateTime = currentTime;
    m = (rand() / float(RAND_MAX)) * 10.0;
    n = (rand() / float(RAND_MAX)) * 10.0;
    printf("m: %f, n: %f\n", m, n);
  }

  LOGI("timeSinceLastFrame: %f", timeSinceLastFrame);

  // Update the particles compute shader program
  glUseProgram(updateParticlesProgramID);
  checkGlError("gl use update Particles Program ");
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_particles_);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_display_);
  checkGlError("bind bufferbase");

  glUniform1f(
      glGetUniformLocation(updateParticlesProgramID, "timeSinceLastFrame"),
      timeSinceLastFrame);
  glUniform1f(glGetUniformLocation(updateParticlesProgramID, "m"), m);
  glUniform1f(glGetUniformLocation(updateParticlesProgramID, "n"), n);

  glDispatchCompute(PARTICLES_COUNT_X / 16, PARTICLES_COUNT_Y / 16, 1);
  checkGlError("Update the particles");

  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
  GLsync Comp_sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
  auto _sync_result = glClientWaitSync(Comp_sync, 0, GL_TIMEOUT_IGNORED);

  // shader program，尝试直接读取以及绘制
  glUseProgram(renderProgramID);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  // 结束的逻辑
  GLsync Reset_sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
  glWaitSync(Reset_sync, 0, GL_TIMEOUT_IGNORED);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
  glUseProgram(0);
  glDeleteSync(Comp_sync);
  glDeleteSync(Reset_sync);
}
bool ComputeShaderParticles::setupGraphics(int w, int h) {
  printGLString("Version", GL_VERSION);
  printGLString("Vendor", GL_VENDOR);
  printGLString("Renderer", GL_RENDERER);
  printGLString("Extensions", GL_EXTENSIONS);

  LOGI("setupGraphics(%d, %d)", w, h);
  // 安卓车机上分辨率为（1886, 1440)，为了展示强行减少到 1440 * 1440，
  w = 1440;
  h = 1440;
  window_width = w;
  window_height = h;

  renderProgramID = createProgram(vertexShaderCode, fragmentShaderCode);
  if (!renderProgramID) {
    LOGE("Could not create render program.");
    return false;
  }

  updateParticlesProgramID =
      createComputeShaderProgram(updateParticlesComputeShaderCode);
  if (!updateParticlesProgramID) {
    LOGE("Could not create update Particles Program.");
    return false;
  }

  createSSBO();
  initSSBOParticlesData();
  initSSBODisplayData();
  isSSBOReady = true;

  genVertexBuffers();

  glViewport(0, 0, w, h);
  checkGlError("glViewport");

  return true;
}
GLuint ComputeShaderParticles::createComputeShaderProgram(
    const char *pComputeSource) {
  LOGI("setup Compute shader");
  GLuint computeShader = loadShader(GL_COMPUTE_SHADER, pComputeSource);
  if (!computeShader) {
    LOGE("can not load computeShader");
    return 0;
  }

  GLuint program = glCreateProgram();
  if (program) {
    glAttachShader(program, computeShader);
    checkGlError("glAttachComputeShader");
    glLinkProgram(program);
    checkGlError("glLinkProgram");
    GLint linkStatus = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if (linkStatus != GL_TRUE) {
      GLint bufLength = 0;
      glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
      if (bufLength) {
        char *buf = (char *)malloc(bufLength);
        if (buf) {
          glGetProgramInfoLog(program, bufLength, NULL, buf);
          LOGE("Could not link program:\n%s\n", buf);
          free(buf);
        }
      }
      glDeleteProgram(program);
      program = 0;
    }
  }
  checkGlError("create program");
  LOGI("Create computeShader success, program = %d", program);

  return program;
}
void ComputeShaderParticles::createSSBO() {
  LOGI("init_ssbo");
  if (ssbo_particles_ != 0) {
    LOGI("SSBO already initVoxelResources");
    return;
  }
  glGenBuffers(1, &ssbo_particles_);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_particles_);
  glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ParticlesBuffer), nullptr,
               GL_DYNAMIC_COPY);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
  checkGlError("initVoxelResources SSBO for particles");

  glGenBuffers(1, &ssbo_display_);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_display_);
  glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ParticlesDisplayBuffer),
               nullptr, GL_DYNAMIC_COPY);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
  checkGlError("initVoxelResources SSBO for display");
}
void ComputeShaderParticles::releaseSSBO() {
  glDeleteBuffers(1, &ssbo_particles_);
  ssbo_particles_ = 0;
  glDeleteBuffers(1, &ssbo_display_);
  ssbo_display_ = 0;
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}
void ComputeShaderParticles::initSSBOParticlesData() {
  LOGI("initVoxelResources particles properties");
  if (ssbo_particles_ == 0) {
    LOGE("particles SSBO not initVoxelResources");
    return;
  }

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_particles_);
  int bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
  ParticlesBuffer *ssbo_particles_buffer =
      static_cast<ParticlesBuffer *>(glMapBufferRange(
          GL_SHADER_STORAGE_BUFFER, 0, sizeof(ParticlesBuffer), bufMask));
  checkGlError("gl Map Buffer Range");
  if (!ssbo_particles_buffer) {
    LOGE("Could not map the SSBO buffer");
    return;
  }

  for (auto i = 0; i < PARTICLES_COUNT_Y; ++i) {
    for (auto j = 0; j < PARTICLES_COUNT_X; ++j) {
      auto pos_w = DISPLAY_X * (rand() / float(RAND_MAX));
      auto pos_h = DISPLAY_Y * (rand() / float(RAND_MAX));
      auto mass = massMin + (massMax - massMin) * (float(rand()) / RAND_MAX);
      ssbo_particles_buffer->particles_properties[i][j].x = pos_w;
      ssbo_particles_buffer->particles_properties[i][j].y = pos_h;
      ssbo_particles_buffer->particles_properties[i][j].z = 0.0;
      ssbo_particles_buffer->particles_properties[i][j].w = 0.0;
      ssbo_particles_buffer->particles_mass[i][j].x = mass;
    }
  }

  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
  glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
  checkGlError("initVoxelResources particle SSBO");

  LOGI("initVoxelResources particles properties done");
}
void ComputeShaderParticles::genVertexBuffers() {
  glGenVertexArrays(1, &vertArray);
  glBindVertexArray(vertArray);

  glGenBuffers(1, &posBuf);
  glBindBuffer(GL_ARRAY_BUFFER, posBuf);
  float data[] = {-1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f};
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 8, data, GL_STREAM_DRAW);
  GLint posPtr = glGetAttribLocation(renderProgramID, "pos");
  glVertexAttribPointer(posPtr, 2, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(posPtr);
}
void ComputeShaderParticles::initSSBODisplayData() {
  LOGI("initVoxelResources display data ");
  if (ssbo_display_ == 0) {
    LOGE("ssbo display not initVoxelResources");
    return;
  }

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_display_);
  int bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
  ParticlesDisplayBuffer *ssbo_display_buffer =
      static_cast<ParticlesDisplayBuffer *>(
          glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0,
                           sizeof(ParticlesDisplayBuffer), bufMask));
  checkGlError("gl Map display Buffer Range");
  if (!ssbo_display_buffer) {
    LOGE("Could not map the SSBO buffer");
    return;
  }

  for (auto i = 0; i < PARTICLES_COUNT_Y; ++i) {
    for (auto j = 0; j < PARTICLES_COUNT_X; ++j) {
      ssbo_display_buffer->particles_count[i][j] = 0;
    }
  }

  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
  glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
  checkGlError("initVoxelResources display SSBO");

  LOGI("initVoxelResources ssbo for display done");
}
