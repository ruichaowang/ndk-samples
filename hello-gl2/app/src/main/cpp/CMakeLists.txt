cmake_minimum_required(VERSION 3.22.1)

# now build app's shared lib
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

add_library(gl2jni SHARED
        ComputeShaderParticles.cpp
            gl_code.cpp)

target_include_directories(gl2jni PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/glm)

# add lib dependencies
target_link_libraries(gl2jni
                      android
                      log 
                      EGL
                      GLESv3)

