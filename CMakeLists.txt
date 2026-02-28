cmake_minimum_required(VERSION 3.16)
project(gametest LANGUAGES CXX)

include(FetchContent)

# C++ Settings
SET(CMAKE_CXX_STANDARD 23)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)
SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

## ----------- PACKAGES -----------
# SDL3
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG release-3.4.2
)
FetchContent_MakeAvailable(SDL3)

# SDL3_image
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    SDL3_image
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
    GIT_TAG release-3.4.0
)
FetchContent_MakeAvailable(SDL3_image)

# GLM
set(DGLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(DBUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.3 
)
FetchContent_MakeAvailable(GLM)

## ----------- PACKAGES -----------

# Linker settings
file(GLOB_RECURSE SRC_FILES src/*.cpp)
add_executable(${PROJECT_NAME} ${SRC_FILES})

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 SDL3_image::SDL3_image glm::glm-header-only)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)

