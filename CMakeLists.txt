cmake_minimum_required(VERSION "3.2")

project(Ondine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_MBCS -DGLM_ENABLE_EXPERIMENTAL")

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.hpp")

find_package(Vulkan)

include(${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake)

find_package(OpenAL CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_path(STB_INCLUDE_DIRS "stb.h")

add_executable(Ondine "${SOURCES}")

target_link_libraries(Ondine PRIVATE "${Vulkan_LIBRARY}")
target_link_libraries(Ondine PRIVATE OpenAL::OpenAL)
target_link_libraries(Ondine PRIVATE imgui::imgui)
target_link_libraries(Ondine PRIVATE glfw)
target_link_libraries(Ondine PRIVATE glm::glm)
target_link_libraries(Ondine PRIVATE assimp::assimp)
target_include_directories(Ondine PRIVATE "${STB_INCLUDE_DIRS}")

if (WIN32)
  target_link_libraries(Ondine PUBLIC "user32.lib" "gdi32.lib" "xinput.lib" "ws2_32.lib" "winmm.lib" "msvcrt.lib" "msvcrtd.lib" "libcmtd.lib" "ucrtd.lib" "shcore.lib")
else (WIN32)
  target_link_libraries(Ondine PUBLIC "pthread" "stdc++fs" "glfw")
endif()

target_compile_definitions(Ondine PUBLIC YONA_PROJECT_ROOT="${CMAKE_SOURCE_DIR}")
