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

#include "config.h"

static Config config;

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
uniform sampler2D sampler;
out vec3 color;
void main() {
  color = texture(sampler, uv).rgb;
}
)glsl";

const auto renderTextureComputeShaderCode = R"glsl(
#version 320 es

precision highp float;
precision highp uimage2D;
layout(binding = 0, rgba32f) writeonly uniform image2D outputTexture;
layout(binding = 1, r32ui) uniform uimage2D particleCountTexture;
layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main() {
    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
    uint count = imageAtomicExchange(particleCountTexture, pos, uint(0));
    float v = float(count) / 15.0;
    imageStore(outputTexture, pos, vec4(v * 1.0 + 0.0, v * 1.0 + 0.0, v * 1.0 + 0.0, 1.0));
}
)glsl";

static auto updateParticlesComputeShaderCode = R"glsl(
#version 320 es
precision highp float;
precision highp int;
precision highp uimage2D;

layout(binding = 1, r32ui) uniform coherent uimage2D particleCountTexture;
layout(binding = 2, r32f) uniform image2D particlePositionTexture_x;
layout(binding = 3, r32f) uniform image2D particlePositionTexture_y;
layout(binding = 4, r32f) uniform image2D particleMassTexture;
layout(local_size_x = 16, local_size_y = 16) in;
uniform float forceMultiplier;
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
                                can_use += 1;
				rets[uint(can_use)] = ivec2(i, j);
			}
		}
	}

	int choice = int(floor(rand(vec2( float(id.x) / w, float(id.y) / h)) * float(can_use)));
	return rets[choice];
}

void main() {
	ivec2 id = ivec2(gl_GlobalInvocationID.xy);
        vec4 v;
        float pos_x = imageLoad(particlePositionTexture_x, id).r;
        float pos_y = imageLoad(particlePositionTexture_y, id).r;
	float mass = imageLoad(particleMassTexture, id).r;
        v.x = pos_x;
        v.y = pos_y;

	// 更新位置
	v.x += v.z * timeSinceLastFrame;
	v.y += v.w * timeSinceLastFrame;

	// 力的方向
	ivec2 gradient = genGradients(ivec2(v.x, v.y));
	// 力的大小
	float vibration = chladni_equation(ivec2(v.x, v.y));
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
	uint count = imageAtomicAdd(particleCountTexture, ivec2(v.x, v.y), uint(1));
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

        imageStore(particlePositionTexture_x, id, vec4(v.x, 0.0, 0.0, 0.0));   //这样存储的话，只有x的值, rgba 受限，不能直接读取，如果用 ssbo，之后怎么绘制呢？
}
)glsl";

const GLfloat gTriangleVertices[] = {0.0f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f};

ComputeShaderParticles::ComputeShaderParticles() {}

ComputeShaderParticles::~ComputeShaderParticles() {}

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
}
bool ComputeShaderParticles::setupGraphics(int w, int h) {
  printGLString("Version", GL_VERSION);
  printGLString("Vendor", GL_VENDOR);
  printGLString("Renderer", GL_RENDERER);
  printGLString("Extensions", GL_EXTENSIONS);

  LOGI("setupGraphics(%d, %d)", w, h);  // （1886, 1440)
  window_width = w;
  window_height = h;

  CreateAllTextures();
  renderProgramID = createProgram(vertexShaderCode, fragmentShaderCode);
  if (!renderProgramID) {
    LOGE("Could not create render program.");
    return false;
  }

  renderTexureProgramID =
      createComputeShaderProgram(renderTextureComputeShaderCode);
  if (!renderTexureProgramID) {
    LOGE("Could not create Texture Program.");
    return false;
  }

  updateParticlesProgramID =
      createComputeShaderProgram(updateParticlesComputeShaderCode);
  if (!renderTexureProgramID) {
    LOGE("Could not create update Particles Program.");
    return false;
  }

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
void ComputeShaderParticles::CreateAllTextures() {
  if (1) {
    auto sz = config.width * config.height;
    float *data = new float[sz * 4];
    for (int i = 0; i < sz; ++i) {
      float c = float(i) / sz;
      data[i * 4 + 0] = c;
      data[i * 4 + 1] = c;
      data[i * 4 + 2] = c;
      data[i * 4 + 3] = 1.0;
    }

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, config.width, config.height, 0,
                 GL_RGBA, GL_FLOAT, data);
    delete[] data;
  }

  if (1) {
    auto sz = config.width * config.height * 4;
    GLuint *data = new GLuint[sz];
    for (int i = 0; i < sz; ++i) data[i] = 2;
    glGenTextures(1, &particleCountTextureID);
    glBindTexture(GL_TEXTURE_2D, particleCountTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, config.width, config.height, 0,
                 GL_RED_INTEGER, GL_UNSIGNED_INT, data);
    delete[] data;
  }

  if (1) {
    int sizeX = config.particleCountX;
    int sizeY = config.particleCountY;

    auto sz = sizeX * sizeY;
    float *data = new float[sz * 4];
    for (int i = 0; i < sz; ++i) {
      data[i * 4 + 0] = config.width * (rand() / float(RAND_MAX));
      data[i * 4 + 1] = config.height * (rand() / float(RAND_MAX));
      data[i * 4 + 2] = 0.0;
      data[i * 4 + 3] = 0.0;
    }

    glGenTextures(1, &particlePositionTextureID);
    glBindTexture(GL_TEXTURE_2D, particlePositionTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, sizeX, sizeY, 0, GL_RGBA,
                 GL_FLOAT, data);
    delete[] data;
  }

  if (1) {
    int sizeX = config.particleCountX;
    int sizeY = config.particleCountY;

    auto sz = sizeX * sizeY;
    float *data = new float[sz];
    for (int i = 0; i < sz; ++i) {
      data[i] = config.massMin +
                (config.massMax - config.massMin) * (float(rand()) / RAND_MAX);
    }

    glGenTextures(1, &particleMassTextureID);
    glBindTexture(GL_TEXTURE_2D, particleMassTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, sizeX, sizeY, 0, GL_RED, GL_FLOAT,
                 data);
    delete[] data;
  }
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
