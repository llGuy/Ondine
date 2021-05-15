cmake_minimum_required(VERSION "3.2")

project(Yona)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSTB_IMAGE_IMPLEMENTATION -D_MBCS -DCIMGUI_DEFINE_ENUMS_AND_STRUCTS -DCURL_STATICLIB")

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.hpp" "dep/imgui/lib/*.cpp")

include_directories("${CMAKE_SOURCE_DIR}/dep/stb")
include_directories("${CMAKE_SOURCE_DIR}/dep/glm")
include_directories("${CMAKE_SOURCE_DIR}/dep/imgui/include")

find_package(Vulkan)

if (NOT WIN32)
  find_package(OpenAL)
endif()

if (NOT WIN32)
  link_libraries("pthread" "stdc++fs")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wformat-security")
endif()

if (OpenAL_FOUND)
  message(STATUS "Found OpenAL package in system ${OPENAL_INCLUDE_DIR} ${OPENAL_LIBRARY}")
  include_directories("${OPENAL_INCLUDE_DIR}")
else()
  # Do something special for Windows as usual
  include_directories("${CMAKE_SOURCE_DIR}/dependencies/openal-soft-bin-include/include")
endif()

if(Vulkan_FOUND)
  message(STATUS "Found Vulkan package in system ${Vulkan_LIBRARY}")
  set(BUILD_CLIENT true)
  include_directories("${Vulkan_INCLUDE_DIRS}")
else(Vulkan_FOUND)
  message(WARNING "Failed to find Vulkan package in system")

  if(WIN32)
    message(STATUS "On Windows, using bundled vulkan version")
    link_directories("${CMAKE_SOURCE_DIR}/dependencies/vulkan/lib")
    include_directories("${CMAKE_SOURCE_DIR}/dependencies/vulkan/include")
    set(BUILD_CLIENT true)
  else(WIN32)
    set(BUILD_CLIENT false)
    message(WARNING "Cannot use bundled vulkan libraries, on Linux - not buliding client")
  endif()
endif()

add_executable(yona "${SOURCES}")
target_link_libraries(yona PUBLIC "${OPENAL_LIBRARY}")
target_link_libraries(yona PUBLIC "${Vulkan_LIBRARY}" "glfw")
target_compile_definitions(yona PUBLIC YONA_PROJECT_ROOT="${CMAKE_SOURCE_DIR}")
