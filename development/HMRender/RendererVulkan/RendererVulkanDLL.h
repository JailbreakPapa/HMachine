/*
 *   Copyright (c) 2023 Watch Dogs LLC
 *   All rights reserved.
 *   You are only allowed access to this code, if given WRITTEN permission by Watch Dogs LLC.
 */
#pragma once

#include <Foundation/Basics.h>
#include <RendererFoundation/RendererFoundationDLL.h>

// Configure the DLL Import/Export Define
#if WD_ENABLED(WD_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_RENDERERVULKAN_LIB
#    define WD_RENDERERVULKAN_DLL WD_DECL_EXPORT
#  else
#    define WD_RENDERERVULKAN_DLL WD_DECL_IMPORT
#  endif
#else
#  define WD_RENDERERVULKAN_DLL
#endif

// Uncomment to log all layout transitions.
//#define VK_LOG_LAYOUT_CHANGES

#define WD_GAL_VULKAN_RELEASE(vulkanObj) \
  do                                     \
  {                                      \
    if ((vulkanObj) != nullptr)          \
    {                                    \
      (vulkanObj)->Release();            \
      (vulkanObj) = nullptr;             \
    }                                    \
  } while (0)

#define VK_ASSERT_DEBUG(code)                                                                                           \
  do                                                                                                                    \
  {                                                                                                                     \
    auto s = (code);                                                                                                    \
    WD_ASSERT_DEBUG(static_cast<vk::Result>(s) == vk::Result::eSuccess, "Vukan call '{0}' failed with: {1} in {2}:{3}", \
      WD_STRINGIZE(code), vk::to_string(static_cast<vk::Result>(s)).data(), WD_SOURCE_FILE, WD_SOURCE_LINE);            \
  } while (false)

#define VK_ASSERT_DEV(code)                                                                                           \
  do                                                                                                                  \
  {                                                                                                                   \
    auto s = (code);                                                                                                  \
    WD_ASSERT_DEV(static_cast<vk::Result>(s) == vk::Result::eSuccess, "Vukan call '{0}' failed with: {1} in {2}:{3}", \
      WD_STRINGIZE(code), vk::to_string(static_cast<vk::Result>(s)).data(), WD_SOURCE_FILE, WD_SOURCE_LINE);          \
  } while (false)

#define VK_LOG_ERROR(code)                                                                                                                                                \
  do                                                                                                                                                                      \
  {                                                                                                                                                                       \
    auto s = (code);                                                                                                                                                      \
    if (static_cast<vk::Result>(s) != vk::Result::eSuccess)                                                                                                               \
    {                                                                                                                                                                     \
      wdLog::Error("Vukan call '{0}' failed with: {1} in {2}:{3}", WD_STRINGIZE(code), vk::to_string(static_cast<vk::Result>(s)).data(), WD_SOURCE_FILE, WD_SOURCE_LINE); \
    }                                                                                                                                                                     \
  } while (false)

#define VK_SUCCEED_OR_RETURN_LOG(code)                                                                                                                                    \
  do                                                                                                                                                                      \
  {                                                                                                                                                                       \
    auto s = (code);                                                                                                                                                      \
    if (static_cast<vk::Result>(s) != vk::Result::eSuccess)                                                                                                               \
    {                                                                                                                                                                     \
      wdLog::Error("Vukan call '{0}' failed with: {1} in {2}:{3}", WD_STRINGIZE(code), vk::to_string(static_cast<vk::Result>(s)).data(), WD_SOURCE_FILE, WD_SOURCE_LINE); \
      return s;                                                                                                                                                           \
    }                                                                                                                                                                     \
  } while (false)

#define VK_SUCCEED_OR_RETURN_WD_FAILURE(code)                                                                                                                             \
  do                                                                                                                                                                      \
  {                                                                                                                                                                       \
    auto s = (code);                                                                                                                                                      \
    if (static_cast<vk::Result>(s) != vk::Result::eSuccess)                                                                                                               \
    {                                                                                                                                                                     \
      wdLog::Error("Vukan call '{0}' failed with: {1} in {2}:{3}", WD_STRINGIZE(code), vk::to_string(static_cast<vk::Result>(s)).data(), WD_SOURCE_FILE, WD_SOURCE_LINE); \
      return WD_FAILURE;                                                                                                                                                  \
    }                                                                                                                                                                     \
  } while (false)
