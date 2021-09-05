#!/bin/sh -e
./vcpkg/bootstrap-vcpkg.sh
./vcpkg/vcpkg install assimp
./vcpkg/vcpkg install glm
./vcpkg/vcpkg install glfw3
./vcpkg/vcpkg install imgui
./vcpkg/vcpkg install openal-soft
./vcpkg/vcpkg install stb
