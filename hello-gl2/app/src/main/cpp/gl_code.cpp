/*
 * Copyright (C) 2009 The Android Open Source Project
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

/* OpenGL ES 3.2
 * 移植 compute shader 的 particles 到 android native，
 * 修改部分：
 * todo 1： texture 更换为 SSBO
 * todo 2: 还需要添加上点击事件
 * CS 的来源： https://github.com/pgeorgiev98/compute-shader-particles
 * */

#include <jni.h>
#include "ComputeShaderParticles.h"
#include <iostream>

std::unique_ptr<ComputeShaderParticles> gComputeShaderParticles;

extern "C" {
JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_init(JNIEnv* env,
                                                              jobject obj,
                                                              jint width,
                                                              jint height);
JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_step(JNIEnv* env,
                                                              jobject obj);
};

JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_init(JNIEnv* env,
                                                              jobject obj,
                                                              jint width,
                                                              jint height) {
  gComputeShaderParticles = std::make_unique<ComputeShaderParticles>();
  gComputeShaderParticles->setupGraphics(width, height);
}

JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_step(JNIEnv* env,
                                                              jobject obj) {
  gComputeShaderParticles->renderFrame();
}
