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

#include "config.h"

static Config config;

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
in vec2 uv;
out vec4 outColor;

layout(std430, binding = 0) buffer SSBO_particles {
  float particles_position_x[1024][1024];
  float particles_position_y[1024][1024];
  float particles_velocity_x[1024][1024];
  float particles_velocity_y[1024][1024];
  float particles_mass[1024][1024];
  int particles_count[1024][1024];
};

void main() {
  int x = int(uv.x * 1024.0);
  int y = int(uv.y * 1024.0);
  int count = particles_count[x][y];
  float v =  float(count) / 15.0;
  particles_count[x][y] = 0;
  outColor = vec4(v * 0.3 + 0.0, v * 0.5 + 0.0, v * 0.5 + 0.0, 1.0);  //填充颜色,参数按照固定值去设定
}
)glsl";

static auto updateParticlesComputeShaderCode = R"glsl(
#version 320 es
precision highp float;
precision highp int;

layout(local_size_x = 16, local_size_y = 16) in;

layout(std430, binding = 0) buffer SSBO_particles {
  float particles_position_x[1024][1024];
  float particles_position_y[1024][1024];
  float particles_velocity_x[1024][1024];
  float particles_velocity_y[1024][1024];
  float particles_mass[1024][1024];
  int particles_count[1024][1024];
};

uniform float timeSinceLastFrame;
uniform float m;
uniform float n;
float w = 1024.0, h = 1024.0;
float PI = 3.14159265359;

float rand(vec2 co) {
	float a = 12.9898;
	float b = 78.233;
	float c = 43758.5453;
	float dt = dot(co.xy ,vec2(a, b));
	float sn = mod(dt, 3.14);
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
        vec4 v;
        v.x = particles_position_x[id.x][id.y];
        v.y = particles_position_y[id.x][id.y];
        v.z = particles_velocity_x[id.x][id.y];
        v.w = particles_velocity_y[id.x][id.y];
        float mass = particles_mass[id.x][id.y];

	// 更新位置
	v.x += v.z * timeSinceLastFrame;
	v.y += v.w * timeSinceLastFrame;

	// 力的方向
	ivec2 gradient = genGradients(ivec2(int(v.x), int(v.y)));
	// 力的大小
	float vibration = chladni_equation(ivec2(int(v.x), int(v.y)));
	// 求出加速度
	float c = (timeSinceLastFrame * (vibration * 60.0 * 20.0)) / mass;
	// 更新下一帧速度
	v.z += c * float(gradient.x);
	v.w += c * float(gradient.y);

        float x = v.x;
	float y = v.y;
	if ((x - y < 3.0 && x - y > -3.0) || (x + y < w + 3.0 && x + y > w - 3.0)) {
		if (rand(vec2(x, y)) < 0.2) {
			float x_amend = rand(vec2(x + 0.1, y + 0.2)) * w;
			float y_amend = rand(vec2(x + 0.3, y + 0.4)) * h;
			v.x = x_amend;
			v.y = y_amend;
		}
	}

        if ((x < 3.0 || x > w - 3.0) || (y < 3.0 || y > h - 3.0)) {
            if (rand(vec2(x, y)) < 0.1) {
                float x_amend = rand(vec2(x + 0.1, y + 0.2)) * w;
                float y_amend = rand(vec2(x + 0.3, y + 0.4)) * h;
                v.x = x_amend;
                v.y = y_amend;
            }
        }

        int count = atomicAdd(particles_count[int(v.x)][int(v.y)], 1);
//        particles_count[int(v.x)][int(v.y)] = count;
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
	if (v.x > 1024.0) { v.x = 1024.0; v.z *= -s; v.w *= s; }
	if (v.y < 0.0) { v.y = 0.0; v.w *= -s; v.z *= s; }
	if (v.y > 1024.0) { v.y = 1024.0; v.w *= -s; v.z *= s; }

        // 保存结果到 SSBO
        particles_position_x[id.x][id.y] = v.x;
        particles_position_y[id.x][id.y] = v.y;
        particles_velocity_x[id.x][id.y] = v.z;
        particles_velocity_y[id.x][id.y] = v.w;

//        particles_count[int(v.x)][int(v.y)] = int(vibration * 15.0);
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
  static float grey;
  grey += 0.01f;
  if (grey > 1.0f) {
    grey = 0.0f;
  }
  glClearColor(grey, grey, grey, 1.0f);
  checkGlError("glClearColor");
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
  //  LOGI("timeSinceLastFrame: %f", timeSinceLastFrame);

  // Update the particles compute shader program
  glUseProgram(updateParticlesProgramID);
  checkGlError("gl use update Particles Program ");
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_particles_);
  int bufMask = GL_MAP_WRITE_BIT | GL_MAP_READ_BIT;
  ParticlesBuffer *ssbo_particles_buffer =
      static_cast<ParticlesBuffer *>(glMapBufferRange(
          GL_SHADER_STORAGE_BUFFER, 0, sizeof(ParticlesBuffer), bufMask));
  if (!ssbo_particles_buffer) {
    checkGlError("gl Map Buffer Range");
    LOGE("Could not map the SSBO buffer");
    return;
  }

  glUniform1f(
      glGetUniformLocation(updateParticlesProgramID, "timeSinceLastFrame"),
      timeSinceLastFrame);
  glUniform1f(glGetUniformLocation(updateParticlesProgramID, "m"), m);
  glUniform1f(glGetUniformLocation(updateParticlesProgramID, "n"), n);
  glDispatchCompute(config.particleCountX / 16, config.particleCountY / 16, 1);
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
  glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
  glUseProgram(0);
  glDeleteSync(Comp_sync);
  glDeleteSync(Reset_sync);
}
bool ComputeShaderParticles::setupGraphics(int w, int h) {
  printGLString("Version", GL_VERSION);
  printGLString("Vendor", GL_VENDOR);
  printGLString("Renderer", GL_RENDERER);
  printGLString("Extensions", GL_EXTENSIONS);

  LOGI("setupGraphics(%d, %d)", w, h);  // （1886, 1440)
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
  genVertexBuffers();

  glViewport(0, 0, w, h);
  checkGlError("glViewport");

  return true;
}
std::string ComputeShaderParticles::formatShaderCode(std::string shaderCode) {
  for (auto v : config.values) {
    std::string name = v.first;
    std::string s = "${" + name + "}";
    for (;;) {
      auto i = shaderCode.find(s);
      if (i == std::string::npos) break;
      shaderCode.replace(i, s.size(), config.getValue(name));
    }
  }
  return shaderCode;
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
    LOGI("SSBO already init");
    return;
  }
  glGenBuffers(1, &ssbo_particles_);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_particles_);
  glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ParticlesBuffer), nullptr,
               GL_DYNAMIC_COPY);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
  checkGlError("init SSBO");

  initParticleProperties();
}
void ComputeShaderParticles::releaseSSBO() {
  glDeleteBuffers(1, &ssbo_particles_);
  ssbo_particles_ = 0;
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}
void ComputeShaderParticles::initParticleProperties() {
  LOGI("init particles properties");
  if (ssbo_particles_ == 0) {
    LOGE("particles SSBO not init");
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

  for (auto i = 0; i < 1024; ++i) {
    for (auto j = 0; j < 1024; ++j) {
      auto pos_w = config.width * (rand() / float(RAND_MAX));
      auto pos_h = config.height * (rand() / float(RAND_MAX));
      auto mass = config.massMin + (config.massMax - config.massMin) *
                                       (float(rand()) / RAND_MAX);
      ssbo_particles_buffer->particles_position_x[i][j] = pos_w;
      ssbo_particles_buffer->particles_position_y[i][j] = pos_h;
      ssbo_particles_buffer->particles_velocity_x[i][j] = 0.0;
      ssbo_particles_buffer->particles_velocity_y[i][j] = 0.0;
      ssbo_particles_buffer->particles_mass[i][j] = mass;
      ssbo_particles_buffer->particles_count[i][j] = 0;

      // 为了测试绘制，直接在这里模拟一个分布并送入,这部分省略
      //      auto count = static_cast<int>(15.0 * (rand() / float(RAND_MAX)));
      //      ssbo_particles_buffer->particles_count[i][j] = count;
    }
  }

  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
  glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
  checkGlError("init particle SSBO");

  isSSBOReady = true;
  LOGI("init particles properties done");
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
