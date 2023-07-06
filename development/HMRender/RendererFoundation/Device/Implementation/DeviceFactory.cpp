#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Device/DeviceFactory.h>

struct CreatorFuncInfo
{
  wdGALDeviceFactory::CreatorFunc m_Func;
  wdString m_sShaderModel;
  wdString m_sShaderCompiler;
};

static wdHashTable<wdString, CreatorFuncInfo> s_CreatorFuncs;

CreatorFuncInfo* GetCreatorFuncInfo(const char* szRendererName)
{
  auto pFuncInfo = s_CreatorFuncs.GetValue(szRendererName);
  if (pFuncInfo == nullptr)
  {
    wdStringBuilder sPluginName = "wdRenderer";
    sPluginName.Append(szRendererName);

    WD_VERIFY(wdPlugin::LoadPlugin(sPluginName).Succeeded(), "Renderer plugin '{}' not found", sPluginName);

    pFuncInfo = s_CreatorFuncs.GetValue(szRendererName);
    WD_ASSERT_DEV(pFuncInfo != nullptr, "Renderer '{}' is not registered", szRendererName);
  }

  return pFuncInfo;
}

wdInternal::NewInstance<wdGALDevice> wdGALDeviceFactory::CreateDevice(const char* szRendererName, wdAllocatorBase* pAllocator, const wdGALDeviceCreationDescription& desc)
{
  if (auto pFuncInfo = GetCreatorFuncInfo(szRendererName))
  {
    return pFuncInfo->m_Func(pAllocator, desc);
  }

  return wdInternal::NewInstance<wdGALDevice>(nullptr, pAllocator);
}

void wdGALDeviceFactory::GetShaderModelAndCompiler(const char* szRendererName, const char*& ref_szShaderModel, const char*& ref_szShaderCompiler)
{
  if (auto pFuncInfo = GetCreatorFuncInfo(szRendererName))
  {
    ref_szShaderModel = pFuncInfo->m_sShaderModel;
    ref_szShaderCompiler = pFuncInfo->m_sShaderCompiler;
  }
}

void wdGALDeviceFactory::RegisterCreatorFunc(const char* szRendererName, const CreatorFunc& func, const char* szShaderModel, const char* szShaderCompiler)
{
  CreatorFuncInfo funcInfo;
  funcInfo.m_Func = func;
  funcInfo.m_sShaderModel = szShaderModel;
  funcInfo.m_sShaderCompiler = szShaderCompiler;

  WD_VERIFY(s_CreatorFuncs.Insert(szRendererName, funcInfo) == false, "Creator func already registered");
}

void wdGALDeviceFactory::UnregisterCreatorFunc(const char* szRendererName)
{
  WD_VERIFY(s_CreatorFuncs.Remove(szRendererName), "Creator func not registered");
}


WD_STATICLINK_FILE(RendererFoundation, RendererFoundation_Device_Implementation_DeviceFactory);
