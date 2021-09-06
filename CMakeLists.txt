cmake_minimum_required(VERSION "3.2")

project(Ondine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_MBCS -DGLM_ENABLE_EXPERIMENTAL")

file(GLOB ONDINE_SOURCES "src/*.cpp" "src/*.hpp")
file(GLOB_RECURSE IMGUI_SOURCES "src/Imgui/*.cpp" "src/Imgui/*.h")

find_package(Vulkan)

include(${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake)

find_package(OpenAL CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_path(STB_INCLUDE_DIRS "stb.h")

add_library(Imgui "${IMGUI_SOURCES}")
target_link_libraries(Imgui PRIVATE glfw)
target_link_libraries(Imgui PRIVATE "${Vulkan_LIBRARY}")

add_executable(Ondine "${ONDINE_SOURCES}")

target_link_libraries(Ondine PRIVATE "${Vulkan_LIBRARY}")
target_link_libraries(Ondine PRIVATE OpenAL::OpenAL)
target_link_libraries(Ondine PRIVATE glfw)
target_link_libraries(Ondine PRIVATE glm::glm)
target_link_libraries(Ondine PRIVATE assimp::assimp)
target_link_libraries(Ondine PRIVATE Imgui)
target_include_directories(Ondine PRIVATE "${STB_INCLUDE_DIRS}")
target_include_directories(Ondine PRIVATE "src/Imgui")

if (WIN32)
  target_link_libraries(Ondine PUBLIC "user32.lib" "gdi32.lib" "xinput.lib" "ws2_32.lib" "winmm.lib" "msvcrt.lib" "msvcrtd.lib" "libcmtd.lib" "ucrtd.lib" "shcore.lib")
else (WIN32)
  target_link_libraries(Ondine PUBLIC "pthread" "stdc++fs" "glfw")
endif()

target_compile_definitions(Ondine PUBLIC YONA_PROJECT_ROOT="${CMAKE_SOURCE_DIR}")
