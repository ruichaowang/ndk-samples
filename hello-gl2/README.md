# Hello GL32

- 此工程原本为 Android C++ sample that draws a triangle using GLES 2.0 API，
- 现在改成了使用 ES 3.2 API，利用了 Compute shader 进行粒子绘制，为方便并没有修改工程名称
- CS 代码来源： https://github.com/pgeorgiev98/compute-shader-particles
- 为了在 ES 上运行，由于rgba32f 兼容性问题，原先有关于 Image 操作的部分被剔除，全部换为SSBO
- 当前运算上会有bug，粒子过一段时间会消失掉，大概和边界处理上有关系

It uses JNI to do the rendering in C++ over a
[GLSurfaceView](http://developer.android.com/reference/android/opengl/GLSurfaceView.html)
created from a regular Android Java Activity.

This sample uses the new
[Android Studio CMake plugin](http://tools.android.com/tech-docs/external-c-builds)
with C++ support.

## Pre-requisites

- Android Studio 2.2 preview+ with [NDK](https://developer.android.com/ndk/)
  bundle.

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
