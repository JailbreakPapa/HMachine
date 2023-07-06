#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/RendererCoreDLL.h>

class wdShaderStageBinary;
struct wdVertexDeclarationInfo;

using wdTexture2DResourceHandle = wdTypedResourceHandle<class wdTexture2DResource>;
using wdRenderToTexture2DResourceHandle = wdTypedResourceHandle<class wdRenderToTexture2DResource>;
using wdTextureCubeResourceHandle = wdTypedResourceHandle<class wdTextureCubeResource>;
using wdMeshBufferResourceHandle = wdTypedResourceHandle<class wdMeshBufferResource>;
using wdDynamicMeshBufferResourceHandle = wdTypedResourceHandle<class wdDynamicMeshBufferResource>;
using wdMeshResourceHandle = wdTypedResourceHandle<class wdMeshResource>;
using wdMaterialResourceHandle = wdTypedResourceHandle<class wdMaterialResource>;
using wdShaderResourceHandle = wdTypedResourceHandle<class wdShaderResource>;
using wdShaderPermutationResourceHandle = wdTypedResourceHandle<class wdShaderPermutationResource>;
using wdRenderPipelineResourceHandle = wdTypedResourceHandle<class wdRenderPipelineResource>;
using wdDecalResourceHandle = wdTypedResourceHandle<class wdDecalResource>;
using wdDecalAtlasResourceHandle = wdTypedResourceHandle<class wdDecalAtlasResource>;

struct WD_RENDERERCORE_DLL wdPermutationVar
{
  WD_DECLARE_MEM_RELOCATABLE_TYPE();

  wdHashedString m_sName;
  wdHashedString m_sValue;

  WD_ALWAYS_INLINE bool operator==(const wdPermutationVar& other) const { return m_sName == other.m_sName && m_sValue == other.m_sValue; }
};
