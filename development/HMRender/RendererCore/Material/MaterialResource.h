#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

using wdMaterialResourceHandle = wdTypedResourceHandle<class wdMaterialResource>;
using wdTexture2DResourceHandle = wdTypedResourceHandle<class wdTexture2DResource>;
using wdTextureCubeResourceHandle = wdTypedResourceHandle<class wdTextureCubeResource>;

struct wdMaterialResourceDescriptor
{
  struct Parameter
  {
    wdHashedString m_Name;
    wdVariant m_Value;

    WD_FORCE_INLINE bool operator==(const Parameter& other) const { return m_Name == other.m_Name && m_Value == other.m_Value; }
  };

  struct Texture2DBinding
  {
    wdHashedString m_Name;
    wdTexture2DResourceHandle m_Value;

    WD_FORCE_INLINE bool operator==(const Texture2DBinding& other) const { return m_Name == other.m_Name && m_Value == other.m_Value; }
  };

  struct TextureCubeBinding
  {
    wdHashedString m_Name;
    wdTextureCubeResourceHandle m_Value;

    WD_FORCE_INLINE bool operator==(const TextureCubeBinding& other) const { return m_Name == other.m_Name && m_Value == other.m_Value; }
  };

  void Clear();

  bool operator==(const wdMaterialResourceDescriptor& other) const;
  WD_FORCE_INLINE bool operator!=(const wdMaterialResourceDescriptor& other) const { return !(*this == other); }

  wdMaterialResourceHandle m_hBaseMaterial;
  // wdSurfaceResource is not linked into this project (not true anymore -> could be changed)
  // this is not used for game purposes but rather for automatic collision mesh generation, so we only store the asset ID here
  wdHashedString m_sSurface;
  wdShaderResourceHandle m_hShader;
  wdDynamicArray<wdPermutationVar> m_PermutationVars;
  wdDynamicArray<Parameter> m_Parameters;
  wdDynamicArray<Texture2DBinding> m_Texture2DBindings;
  wdDynamicArray<TextureCubeBinding> m_TextureCubeBindings;
};

class WD_RENDERERCORE_DLL wdMaterialResource final : public wdResource
{
  WD_ADD_DYNAMIC_REFLECTION(wdMaterialResource, wdResource);
  WD_RESOURCE_DECLARE_COMMON_CODE(wdMaterialResource);
  WD_RESOURCE_DECLARE_CREATEABLE(wdMaterialResource, wdMaterialResourceDescriptor);

public:
  wdMaterialResource();
  ~wdMaterialResource();

  wdHashedString GetPermutationValue(const wdTempHashedString& sName);
  wdHashedString GetSurface() const;

  void SetParameter(const wdHashedString& sName, const wdVariant& value);
  void SetParameter(const char* szName, const wdVariant& value);
  wdVariant GetParameter(const wdTempHashedString& sName);

  void SetTexture2DBinding(const wdHashedString& sName, const wdTexture2DResourceHandle& value);
  void SetTexture2DBinding(const char* szName, const wdTexture2DResourceHandle& value);
  wdTexture2DResourceHandle GetTexture2DBinding(const wdTempHashedString& sName);

  void SetTextureCubeBinding(const wdHashedString& sName, const wdTextureCubeResourceHandle& value);
  void SetTextureCubeBinding(const char* szName, const wdTextureCubeResourceHandle& value);
  wdTextureCubeResourceHandle GetTextureCubeBinding(const wdTempHashedString& sName);

  /// \brief Copies current desc to original desc so the material is not modified on reset
  void PreserveCurrentDesc();
  virtual void ResetResource() override;

  const wdMaterialResourceDescriptor& GetCurrentDesc() const;

  /// \brief Use these enum values together with GetDefaultMaterialFileName() to get the default file names for these material types.
  enum class DefaultMaterialType
  {
    Fullbright,
    FullbrightAlphaTest,
    Lit,
    LitAlphaTest,
    Sky,
    MissingMaterial
  };

  /// \brief Returns the default material file name for the given type (materials in Data/Base/Materials/BaseMaterials).
  static const char* GetDefaultMaterialFileName(DefaultMaterialType materialType);

private:
  virtual wdResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual wdResourceLoadDesc UpdateContent(wdStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  wdMaterialResourceDescriptor m_mOriginalDesc; // stores the state at loading, such that SetParameter etc. calls can be reset later
  wdMaterialResourceDescriptor m_mDesc;

  friend class wdRenderContext;
  WD_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererCore, MaterialResource);

  wdEvent<const wdMaterialResource*, wdMutex> m_ModifiedEvent;
  void OnBaseMaterialModified(const wdMaterialResource* pModifiedMaterial);
  void OnResourceEvent(const wdResourceEvent& resourceEvent);

  void AddPermutationVar(const char* szName, const char* szValue);

  wdAtomicInteger32 m_iLastModified;
  wdAtomicInteger32 m_iLastConstantsModified;
  wdInt32 m_iLastUpdated;
  wdInt32 m_iLastConstantsUpdated;

  bool IsModified();
  bool AreConstantsModified();

  void UpdateConstantBuffer(wdShaderPermutationResource* pShaderPermutation);

  wdConstantBufferStorageHandle m_hConstantBufferStorage;

  struct CachedValues
  {
    wdShaderResourceHandle m_hShader;
    wdHashTable<wdHashedString, wdHashedString> m_PermutationVars;
    wdHashTable<wdHashedString, wdVariant> m_Parameters;
    wdHashTable<wdHashedString, wdTexture2DResourceHandle> m_Texture2DBindings;
    wdHashTable<wdHashedString, wdTextureCubeResourceHandle> m_TextureCubeBindings;
  };

  wdUInt32 m_uiCacheIndex;
  CachedValues* m_pCachedValues;

  CachedValues* GetOrUpdateCachedValues();
  CachedValues* AllocateCache();
  void DeallocateCache(wdUInt32 uiCacheIndex);

  wdMutex m_UpdateCacheMutex;
  static wdDeque<wdMaterialResource::CachedValues> s_CachedValues;

  static void ClearCache();
};
