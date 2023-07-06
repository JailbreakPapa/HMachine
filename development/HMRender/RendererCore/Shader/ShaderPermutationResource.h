#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Time/Timestamp.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererCore/Shader/ShaderPermutationBinary.h>
#include <RendererCore/ShaderCompiler/PermutationGenerator.h>

using wdShaderPermutationResourceHandle = wdTypedResourceHandle<class wdShaderPermutationResource>;
using wdShaderStateResourceHandle = wdTypedResourceHandle<class wdShaderStateResource>;

struct wdShaderPermutationResourceDescriptor
{
};

class WD_RENDERERCORE_DLL wdShaderPermutationResource : public wdResource
{
  WD_ADD_DYNAMIC_REFLECTION(wdShaderPermutationResource, wdResource);
  WD_RESOURCE_DECLARE_COMMON_CODE(wdShaderPermutationResource);
  WD_RESOURCE_DECLARE_CREATEABLE(wdShaderPermutationResource, wdShaderPermutationResourceDescriptor);

public:
  wdShaderPermutationResource();

  wdGALShaderHandle GetGALShader() const { return m_hShader; }
  const wdShaderStageBinary* GetShaderStageBinary(wdGALShaderStage::Enum stage) const { return m_pShaderStageBinaries[stage]; }

  wdGALBlendStateHandle GetBlendState() const { return m_hBlendState; }
  wdGALDepthStencilStateHandle GetDepthStencilState() const { return m_hDepthStencilState; }
  wdGALRasterizerStateHandle GetRasterizerState() const { return m_hRasterizerState; }

  bool IsShaderValid() const { return m_bShaderPermutationValid; }

  wdArrayPtr<const wdPermutationVar> GetPermutationVars() const { return m_PermutationVars; }

private:
  virtual wdResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual wdResourceLoadDesc UpdateContent(wdStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;
  virtual wdResourceTypeLoader* GetDefaultResourceTypeLoader() const override;

private:
  friend class wdShaderManager;

  wdShaderStageBinary* m_pShaderStageBinaries[wdGALShaderStage::ENUM_COUNT];

  bool m_bShaderPermutationValid;
  wdGALShaderHandle m_hShader;

  wdGALBlendStateHandle m_hBlendState;
  wdGALDepthStencilStateHandle m_hDepthStencilState;
  wdGALRasterizerStateHandle m_hRasterizerState;

  wdHybridArray<wdPermutationVar, 16> m_PermutationVars;
};


class wdShaderPermutationResourceLoader : public wdResourceTypeLoader
{
public:
  virtual wdResourceLoadData OpenDataStream(const wdResource* pResource) override;
  virtual void CloseDataStream(const wdResource* pResource, const wdResourceLoadData& loaderData) override;

  virtual bool IsResourceOutdated(const wdResource* pResource) const override;

private:
  wdResult RunCompiler(const wdResource* pResource, wdShaderPermutationBinary& BinaryInfo, bool bForce);
};
