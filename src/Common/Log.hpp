#pragma once

#include <imgui_console/imgui_console.h>

#define LOG_TO_CONSOLE

extern ImGuiConsole *gConsole;
extern char gCommandBuffer[4096];

#if defined (LOG_TO_CONSOLE)

#define LOG_ERRORV(str, ...)                    \
  if (gConsole) { \
  std::snprintf(gCommandBuffer, sizeof(gCommandBuffer), str, __VA_ARGS__); \
  gConsole->System().Log(csys::ItemType::ERROR) << "(" << __FUNCTION__ << ") " << gCommandBuffer; }

#define LOG_WARNINGV(str, ...)                    \
  if (gConsole) { \
  std::snprintf(gCommandBuffer, sizeof(gCommandBuffer), str, __VA_ARGS__); \
  gConsole->System().Log(csys::ItemType::WARNING) << "(" << __FUNCTION__ << ") " << gCommandBuffer; }

#define LOG_INFOV(str, ...)                    \
  if (gConsole) { \
  std::snprintf(gCommandBuffer, sizeof(gCommandBuffer), str, __VA_ARGS__); \
  gConsole->System().Log(csys::ItemType::INFO) << "(" << __FUNCTION__ << ") " << gCommandBuffer; }

#define LOG_ERROR(str)                    \
  if (gConsole) { \
  gConsole->System().Log(csys::ItemType::ERROR) << "(" << __FUNCTION__ << ") " << str; }

#define LOG_WARNING(str)                    \
  if (gConsole) { \
  gConsole->System().Log(csys::ItemType::WARNING) << "(" << __FUNCTION__ << ") " << str; }

#define LOG_INFO(str, ...)                    \
  if (gConsole) { \
  gConsole->System().Log(csys::ItemType::INFO) << "(" << __FUNCTION__ << ") " << str; }

#else

#define LOG_ERRORV(str, ...)                    \
  printf("ERROR:%s: ", __FUNCTION__);           \
  printf(str, __VA_ARGS__)

#define LOG_WARNINGV(str, ...)                  \
  printf("WARNING:%s: ", __FUNCTION__);         \
  printf(str, __VA_ARGS__)

#define LOG_INFOV(str, ...)                     \
  printf("INFO:%s: ", __FUNCTION__);            \
  printf(str, __VA_ARGS__)

#define LOG_ERROR(str)                          \
  printf("ERROR:%s: ", __FUNCTION__);           \
  printf(str)

#define LOG_WARNING(str)                        \
  printf("WARNING:%s: ", __FUNCTION__);         \
  printf(str)

#define LOG_INFO(str)                           \
  printf("INFO:%s: ", __FUNCTION__);            \
  printf(str)

#endif
