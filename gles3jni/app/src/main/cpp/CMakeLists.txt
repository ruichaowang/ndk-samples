cmake_minimum_required(VERSION 3.22.1)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions -Wall")
set(OPENGL_LIB GLESv3)

add_library(gles3jni SHARED
            ${GL3STUB_SRC}
            gles3jni.cpp)

include_directories(./glm)

# Include libraries needed for gles3jni lib
target_link_libraries(gles3jni
            ${OPENGL_LIB}
            android
            EGL
            log
            m)
