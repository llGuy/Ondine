cmake_minimum_required(VERSION "3.2")

project(Ondine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSTB_IMAGE_IMPLEMENTATION -D_MBCS -DGLM_ENABLE_EXPERIMENTAL")

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.hpp" "dep/imgui/lib/*.cpp")

include_directories("${CMAKE_SOURCE_DIR}/dep/stb")
include_directories("${CMAKE_SOURCE_DIR}/dep/glm")
include_directories("${CMAKE_SOURCE_DIR}/dep/imgui/include")

find_package(Vulkan)
find_package(OpenAL)

add_executable(Ondine "${SOURCES}")

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
  target_link_libraries(Ondine PUBLIC "${Vulkan_LIBRARY}")

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

  target_link_libraries(Ondine PUBLIC "pthread" "stdc++fs" "glfw")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

endif()


target_compile_definitions(Ondine PUBLIC YONA_PROJECT_ROOT="${CMAKE_SOURCE_DIR}")
