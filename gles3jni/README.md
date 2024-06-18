# gles3jni

## 工程说明：
- 利用了 ES3  instance drawing 等特性来提高渲染效率, 为了方便修改代码删除了虚函数和重载；
- 目的是 render Voxels and 3D reconstruction with 2D texturing
- 图像数据，摄像头数据，来源于：NuScenes https://www.nuscenes.org/
- voxels prediction 来源于：TPVFormer https://github.com/wzzheng/tpvformer
- M1 Mac 运行的 GL 版本：https://github.com/ruichaowang/Learning_GL/tree/main

## 资源消耗

- original sample( 16*16 instance): 8155: 5% CPU, 20~22% GPU(不运行时 16%); 8295:7.6% CPU, 0.75～1%
- 8295 256*256 instance: 15% CPU, 7.14% GPU, 512*512 instance: 37% CPU,26.73 GPU% , 1024*1024 45% CPU, 60% GPU
- 

## 原始说明

gles3jni is an Android C++ sample that demonstrates how to use OpenGL ES 3.0
from JNI/native code.

The OpenGL ES 3.0 rendering path uses a few new features compared to the OpenGL
ES 2.0 path:

- Instanced rendering and vertex attribute divisor to reduce the number of draw
  calls and uniform changes.
- Vertex array objects to reduce the number of calls required to set up vertex
  attribute state on each frame.
- Explicit assignment of attribute locations, eliminating the need to query
  assignments.

This sample uses the new
[Android Studio CMake plugin](http://tools.android.com/tech-docs/external-c-builds)
with C++ support.

## Pre-requisites

- Android Studio 1.3+ with [NDK](https://developer.android.com/ndk/) bundle.

## Getting Started

1. [Download Android Studio](http://developer.android.com/sdk/index.html)
1. Launch Android Studio.
1. Open the sample directory.
1. Open *File/Project Structure...*

- Click *Download* or *Select NDK location*.

1. Click *Tools/Android/Sync Project with Gradle Files*.
1. Click *Run/Run 'app'*.

## Screenshots

![screenshot](screenshot.png)

## Support

If you've found an error in these samples, please
[file an issue](https://github.com/googlesamples/android-ndk/issues/new).

Patches are encouraged, and may be submitted by
[forking this project](https://github.com/googlesamples/android-ndk/fork) and
submitting a pull request through GitHub. Please see
[CONTRIBUTING.md](../CONTRIBUTING.md) for more details.

- [Stack Overflow](http://stackoverflow.com/questions/tagged/android-ndk)
- [Android Tools Feedbacks](http://tools.android.com/feedback)

## License

Copyright 2015 Google, Inc.

Licensed to the Apache Software Foundation (ASF) under one or more contributor
license agreements. See the NOTICE file distributed with this work for
additional information regarding copyright ownership. The ASF licenses this file
to you under the Apache License, Version 2.0 (the "License"); you may not use
this file except in compliance with the License. You may obtain a copy of the
License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed
under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND, either express or implied. See the License for the
specific language governing permissions and limitations under the License.
