cmake_minimum_required(VERSION 3.22.1)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -UNDEBUG")

add_library(native-codec-jni SHARED
            looper.cpp
            native-codec-jni.cpp)

# Include libraries needed for native-codec-jni lib
target_link_libraries(native-codec-jni
                      android
                      log
                      mediandk
                      OpenMAXAL)

