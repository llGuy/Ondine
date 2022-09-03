cmake_minimum_required(VERSION "3.2")

project(Ondine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_MBCS -DGLM_ENABLE_EXPERIMENTAL -Wordered-compare-function-pointers")

file(GLOB_RECURSE COMMON_SOURCES "src/Common/*.cpp" "src/Common/*.hpp")
file(GLOB_RECURSE CORE_SOURCES "src/Core/*.cpp" "src/Core/*.hpp")
file(GLOB_RECURSE GRAPHICS_SOURCES "src/Graphics/*.cpp" "src/Graphics/*.hpp")
file(GLOB_RECURSE RUNTIME_SOURCES "src/Runtime/*.cpp" "src/Runtime/*.hpp")
file(GLOB_RECURSE VIEW_SOURCES "src/View/*.cpp" "src/View/*.hpp")
file(GLOB_RECURSE IMGUI_SOURCES "src/Imgui/*.cpp" "src/Imgui/*.h")

include_directories("${CMAKE_SOURCE_DIR}/dep/stb")
include_directories("${CMAKE_SOURCE_DIR}/dep/glm")
include_directories("${CMAKE_SOURCE_DIR}/dep/imgui/include")

find_package(Vulkan)
find_package(OpenAL)

set(ASSIMP_BUILD_FBX_IMPORTER ON)
set(ASSIMP_BUILD_FBX_EXPORTER ON)
set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
set(ASSIMP_BUILD_TESTS OFF)
add_subdirectory("${CMAKE_SOURCE_DIR}/dep/assimp")

add_library(Imgui "${IMGUI_SOURCES}")
target_include_directories(Imgui PUBLIC "${Vulkan_INCLUDE_DIRS}")
target_link_libraries(Imgui PUBLIC "${Vulkan_LIBRARY}" "glfw")

include_directories("src/Common")
include_directories("src/Core")
include_directories("src/Graphics")
include_directories("src/View")
include_directories("src/Imgui")

add_library(Common "${COMMON_SOURCES}")
add_library(Core "${CORE_SOURCES}")
target_link_libraries(Common PUBLIC Core)
add_library(Graphics "${GRAPHICS_SOURCES}")
target_link_libraries(Graphics PUBLIC "${Vulkan_LIBRARY}")
target_link_libraries(Graphics PUBLIC Imgui)
target_link_libraries(Graphics PUBLIC Common)
target_link_libraries(Graphics PUBLIC "assimp")
target_include_directories(Graphics PUBLIC "${CMAKE_SOURCE_DIR}/dep/assimp/include")
target_include_directories(Graphics PUBLIC "src/Imgui")

add_library(View "${VIEW_SOURCES}")
target_link_libraries(View PUBLIC "assimp" Imgui Core Graphics)
target_include_directories(View PUBLIC "${CMAKE_SOURCE_DIR}/dep/assimp/include")

add_executable(Ondine "${RUNTIME_SOURCES}")

target_link_libraries(Ondine PUBLIC Common)
target_link_libraries(Ondine PUBLIC Core)
target_link_libraries(Ondine PUBLIC Graphics)
target_link_libraries(Ondine PUBLIC View)

# OpenAL
if (OpenAL_FOUND)

  message(STATUS "Found OpenAL package in system ${OPENAL_INCLUDE_DIR} ${OPENAL_LIBRARY}")
  target_include_directories(Ondine PUBLIC "${OPENAL_INCLUDE_DIR}")
  target_link_libraries(Ondine PUBLIC "${OPENAL_LIBRARY}")

else()

  # Do something special for Windows as usual
  if (WIN32)
    target_link_libraries(Ondine PUBLIC "${CMAKE_SOURCE_DIR}/dep/openal-soft-bin-include/bin/Debug/OpenAL32.lib")
    target_include_directories(Ondine PUBLIC "${CMAKE_SOURCE_DIR}/dep/openal-soft-bin-include/include")
  endif()

endif()

# Vulkan
if(Vulkan_FOUND)

  message(STATUS "Found Vulkan package in system ${Vulkan_LIBRARY}")
  target_include_directories(Ondine PUBLIC "${Vulkan_INCLUDE_DIRS}")
  target_include_directories(Core PUBLIC "${Vulkan_INCLUDE_DIRS}")
  target_include_directories(View PUBLIC "${Vulkan_INCLUDE_DIRS}")
  message("${Vulkan_INCLUDE_DIRS}")
  target_link_libraries(Ondine PUBLIC "${Vulkan_LIBRARY}")
  target_link_libraries(Core PUBLIC "${Vulkan_LIBRARY}")
  target_link_libraries(View PUBLIC "${Vulkan_LIBRARY}")

else(Vulkan_FOUND)

  message(WARNING "Failed to find Vulkan package in system")

  if(WIN32)

    message(STATUS "On Windows, using bundled vulkan version")
    target_link_directories(Ondine PUBLIC "${CMAKE_SOURCE_DIR}/dep/vulkan/lib")
    target_include_directories(Ondine PUBLIC "${CMAKE_SOURCE_DIR}/dep/vulkan/include")

  else(WIN32)

    message(WARNING "Cannot use bundled vulkan libraries, on Linux - not buliding client")

  endif()

endif()


if (WIN32)

  target_link_directories(Ondine PUBLIC "${CMAKE_SOURCE_DIR}/dep/glfw/lib")
  target_link_libraries(Ondine PUBLIC "glfw3.lib" "user32.lib" "gdi32.lib" "xinput.lib" "ws2_32.lib" "winmm.lib" "msvcrt.lib" "msvcrtd.lib" "libcmtd.lib" "ucrtd.lib" "shcore.lib")
  target_include_directories(Ondine PUBLIC "${CMAKE_SOURCE_DIR}/dep/glfw/include")

else (WIN32)

  # target_link_libraries(Ondine PUBLIC "pthread" "stdc++fs" "glfw")
  # target_link_libraries(Core PUBLIC "pthread" "stdc++fs" "glfw")
  target_link_libraries(Ondine PUBLIC "pthread" "glfw")
  target_link_libraries(Core PUBLIC "pthread" "glfw")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

endif()

target_link_libraries(Ondine PUBLIC "assimp")
target_include_directories(Ondine PUBLIC "${CMAKE_SOURCE_DIR}/dep/assimp/include")

target_compile_definitions(Ondine PUBLIC YONA_PROJECT_ROOT="${CMAKE_SOURCE_DIR}")
