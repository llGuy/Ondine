cmake_minimum_required(VERSION "3.2")

project(Ondine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_MBCS -DGLM_ENABLE_EXPERIMENTAL")

# Prepare all source files
file(GLOB_RECURSE COMMON_SOURCES "src/Common/*.cpp" "src/Common/*.hpp")
file(GLOB_RECURSE CORE_SOURCES "src/Core/*.cpp" "src/Core/*.hpp")
file(GLOB_RECURSE GRAPHICS_SOURCES "src/Graphics/*.cpp" "src/Graphics/*.hpp")
file(GLOB_RECURSE RUNTIME_SOURCES "src/Runtime/*.cpp" "src/Runtime/*.hpp")
file(GLOB_RECURSE VIEW_SOURCES "src/View/*.cpp" "src/View/*.hpp")
file(GLOB_RECURSE IMGUI_SOURCES "src/Imgui/*.cpp" "src/Imgui/*.h")

# Get all packages / dependencies
find_package(Vulkan)

include(${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake)

find_package(OpenAL CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_path(STB_INCLUDE_DIRS "stb.h")

# Build ImGUI from docking branch with vulkan backend
add_library(Imgui "${IMGUI_SOURCES}")
# target_link_libraries(Imgui PRIVATE glfw)
# target_link_libraries(Imgui PRIVATE "${Vulkan_LIBRARY}")

include_directories("src/Common")
include_directories("src/Core")
include_directories("src/Graphics")
include_directories("src/View")
include_directories("src/Imgui")

# Build libraries
add_library(Common "${COMMON_SOURCES}")
target_include_directories(Common PUBLIC "${STB_INCLUDE_DIRS}")

add_library(Core "${CORE_SOURCES}")
target_link_libraries(Core PUBLIC glm::glm)

add_library(Graphics "${GRAPHICS_SOURCES}")
target_link_libraries(Graphics PUBLIC glm::glm)
target_link_libraries(Graphics PUBLIC "${Vulkan_LIBRARY}")
target_link_libraries(Graphics PUBLIC Imgui)
target_link_libraries(Graphics PUBLIC Common)
target_include_directories(Graphics PUBLIC "src/Imgui")

add_library(View "${VIEW_SOURCES}")
target_link_libraries(View PUBLIC glm::glm)

add_executable(Ondine "${RUNTIME_SOURCES}")

target_link_libraries(Ondine PUBLIC OpenAL::OpenAL)
target_link_libraries(Ondine PUBLIC glfw)
target_link_libraries(Ondine PUBLIC glm::glm)
target_link_libraries(Ondine PUBLIC assimp::assimp)
target_link_libraries(Ondine PUBLIC Common)
target_link_libraries(Ondine PUBLIC Core)
target_link_libraries(Ondine PUBLIC Graphics)
target_link_libraries(Ondine PUBLIC View)
target_include_directories(Ondine PUBLIC "${STB_INCLUDE_DIRS}")

if (WIN32)
  target_link_libraries(Ondine PUBLIC "user32.lib" "gdi32.lib" "xinput.lib" "ws2_32.lib" "winmm.lib" "msvcrt.lib" "msvcrtd.lib" "libcmtd.lib" "ucrtd.lib" "shcore.lib")
else (WIN32)
  target_link_libraries(Ondine PUBLIC "pthread" "stdc++fs" "glfw")
endif()

target_compile_definitions(Ondine PUBLIC YONA_PROJECT_ROOT="${CMAKE_SOURCE_DIR}")
