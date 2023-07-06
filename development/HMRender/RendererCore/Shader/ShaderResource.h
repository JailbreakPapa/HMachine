#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/RendererCoreDLL.h>

using wdShaderResourceHandle = wdTypedResourceHandle<class wdShaderResource>;

struct wdShaderResourceDescriptor
{
};

class WD_RENDERERCORE_DLL wdShaderResource : public wdResource
{
  WD_ADD_DYNAMIC_REFLECTION(wdShaderResource, wdResource);
  WD_RESOURCE_DECLARE_COMMON_CODE(wdShaderResource);
  WD_RESOURCE_DECLARE_CREATEABLE(wdShaderResource, wdShaderResourceDescriptor);

public:
  wdShaderResource();

  bool IsShaderValid() const { return m_bShaderResourceIsValid; }

  wdArrayPtr<const wdHashedString> GetUsedPermutationVars() const { return m_PermutationVarsUsed; }

private:
  virtual wdResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual wdResourceLoadDesc UpdateContent(wdStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  wdHybridArray<wdHashedString, 16> m_PermutationVarsUsed;
  bool m_bShaderResourceIsValid;
};
