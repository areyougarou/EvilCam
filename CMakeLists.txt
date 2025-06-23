cmake_minimum_required(VERSION 3.22.1)

project("magiccam")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add compiler flags for optimization and security
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -fvisibility=hidden -ffunction-sections -fdata-sections -fno-rtti -fno-exceptions")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections -Wl,--strip-all -s")

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/jni/include)

# Source files
file(GLOB_RECURSE SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/jni/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/jni/src/*.c"
)

# Create the library
add_library(${CMAKE_PROJECT_NAME} SHARED ${SOURCES})

# Link libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    android
    log
    camera2ndk
    mediandk
    dl
)