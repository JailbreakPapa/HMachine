#pragma once

#include <Foundation/Types/Delegate.h>
#include <RendererFoundation/RendererFoundationDLL.h>

struct WD_RENDERERFOUNDATION_DLL wdGALDeviceFactory
{
  using CreatorFunc = wdDelegate<wdInternal::NewInstance<wdGALDevice>(wdAllocatorBase*, const wdGALDeviceCreationDescription&)>;

  static wdInternal::NewInstance<wdGALDevice> CreateDevice(const char* szRendererName, wdAllocatorBase* pAllocator, const wdGALDeviceCreationDescription& desc);

  static void GetShaderModelAndCompiler(const char* szRendererName, const char*& ref_szShaderModel, const char*& ref_szShaderCompiler);

  static void RegisterCreatorFunc(const char* szRendererName, const CreatorFunc& func, const char* szShaderModel, const char* szShaderCompiler);
  static void UnregisterCreatorFunc(const char* szRendererName);
};
