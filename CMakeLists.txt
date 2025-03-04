cmake_minimum_required(VERSION 3.10)
project(max_nx)

# Set the compiler to the ARM64 cross-compiler
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

# Set the compilation flags
set(CMAKE_C_FLAGS "-Wall -O2 -march=armv8-a")
set(CMAKE_CXX_FLAGS "-Wall -O2 -march=armv8-a")

# Include directories
include_directories(include)
include_directories(${CMAKE_SOURCE_DIR})

# Find the required packages
find_package(OpenGL REQUIRED)
find_package(EGL REQUIRED)
find_package(GLESv2 REQUIRED)
find_package(Threads REQUIRED)

# Source files
file(GLOB SOURCES "source/*.c")

# Add executable
add_executable(max_nx ${SOURCES})

# Link libraries
target_link_libraries(max_nx ${OPENGL_LIBRARIES} ${EGL_LIBRARIES} ${GLESv2_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

# If necessary, add any additional libraries or dependencies here
# target_link_libraries(max_nx <additional_libraries>)