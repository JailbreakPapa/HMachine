#include <RendererCore/RendererCorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/Types/ScopeExit.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Shader/ShaderPermutationResource.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererCore/Textures/TextureLoader.h>
#include <Texture/Image/Formats/DdsFileFormat.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
#  include <Foundation/IO/CompressedStreamZstd.h>
#endif

void wdMaterialResourceDescriptor::Clear()
{
  m_hBaseMaterial.Invalidate();
  m_sSurface.Clear();
  m_hShader.Invalidate();
  m_PermutationVars.Clear();
  m_Parameters.Clear();
  m_Texture2DBindings.Clear();
  m_TextureCubeBindings.Clear();
}

bool wdMaterialResourceDescriptor::operator==(const wdMaterialResourceDescriptor& other) const
{
  return m_hBaseMaterial == other.m_hBaseMaterial && m_hShader == other.m_hShader && m_PermutationVars == other.m_PermutationVars && m_Parameters == other.m_Parameters && m_Texture2DBindings == other.m_Texture2DBindings && m_TextureCubeBindings == other.m_TextureCubeBindings;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdMaterialResource, 1, wdRTTIDefaultAllocator<wdMaterialResource>)
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_RESOURCE_IMPLEMENT_COMMON_CODE(wdMaterialResource);
// clang-format on

// clang-format off
WD_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, MaterialResource)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    wdMaterialResource::ClearCache();
  }

WD_END_SUBSYSTEM_DECLARATION;
// clang-format on

wdDeque<wdMaterialResource::CachedValues> wdMaterialResource::s_CachedValues;

wdMaterialResource::wdMaterialResource()
  : wdResource(DoUpdate::OnAnyThread, 1)
{
  m_iLastUpdated = 0;
  m_iLastConstantsUpdated = 0;
  m_uiCacheIndex = wdInvalidIndex;
  m_pCachedValues = nullptr;

  wdResourceManager::GetResourceEvents().AddEventHandler(wdMakeDelegate(&wdMaterialResource::OnResourceEvent, this));
}

wdMaterialResource::~wdMaterialResource()
{
  wdResourceManager::GetResourceEvents().RemoveEventHandler(wdMakeDelegate(&wdMaterialResource::OnResourceEvent, this));
}

wdHashedString wdMaterialResource::GetPermutationValue(const wdTempHashedString& sName)
{
  auto pCachedValues = GetOrUpdateCachedValues();

  wdHashedString sResult;
  pCachedValues->m_PermutationVars.TryGetValue(sName, sResult);

  return sResult;
}

wdHashedString wdMaterialResource::GetSurface() const
{
  if (!m_mDesc.m_sSurface.IsEmpty())
    return m_mDesc.m_sSurface;

  if (m_mDesc.m_hBaseMaterial.IsValid())
  {
    wdResourceLock<wdMaterialResource> pBaseMaterial(m_mDesc.m_hBaseMaterial, wdResourceAcquireMode::BlockTillLoaded);
    return pBaseMaterial->GetSurface();
  }

  return wdHashedString();
}

void wdMaterialResource::SetParameter(const wdHashedString& sName, const wdVariant& value)
{
  wdUInt32 uiIndex = wdInvalidIndex;
  for (wdUInt32 i = 0; i < m_mDesc.m_Parameters.GetCount(); ++i)
  {
    if (m_mDesc.m_Parameters[i].m_Name == sName)
    {
      uiIndex = i;
      break;
    }
  }

  if (value.IsValid())
  {
    if (uiIndex != wdInvalidIndex)
    {
      if (m_mDesc.m_Parameters[uiIndex].m_Value == value)
      {
        return;
      }

      m_mDesc.m_Parameters[uiIndex].m_Value = value;
    }
    else
    {
      auto& param = m_mDesc.m_Parameters.ExpandAndGetRef();
      param.m_Name = sName;
      param.m_Value = value;
    }
  }
  else
  {
    if (uiIndex == wdInvalidIndex)
    {
      return;
    }

    m_mDesc.m_Parameters.RemoveAtAndSwap(uiIndex);
  }

  m_iLastModified.Increment();
  m_iLastConstantsModified.Increment();

  m_ModifiedEvent.Broadcast(this);
}

void wdMaterialResource::SetParameter(const char* szName, const wdVariant& value)
{
  wdTempHashedString sName(szName);

  wdUInt32 uiIndex = wdInvalidIndex;
  for (wdUInt32 i = 0; i < m_mDesc.m_Parameters.GetCount(); ++i)
  {
    if (m_mDesc.m_Parameters[i].m_Name == sName)
    {
      uiIndex = i;
      break;
    }
  }

  if (value.IsValid())
  {
    if (uiIndex != wdInvalidIndex)
    {
      if (m_mDesc.m_Parameters[uiIndex].m_Value == value)
      {
        return;
      }

      m_mDesc.m_Parameters[uiIndex].m_Value = value;
    }
    else
    {
      auto& param = m_mDesc.m_Parameters.ExpandAndGetRef();
      param.m_Name.Assign(szName);
      param.m_Value = value;
    }
  }
  else
  {
    if (uiIndex == wdInvalidIndex)
    {
      return;
    }

    m_mDesc.m_Parameters.RemoveAtAndSwap(uiIndex);
  }

  m_iLastModified.Increment();
  m_iLastConstantsModified.Increment();

  m_ModifiedEvent.Broadcast(this);
}

wdVariant wdMaterialResource::GetParameter(const wdTempHashedString& sName)
{
  auto pCachedValues = GetOrUpdateCachedValues();

  wdVariant value;
  pCachedValues->m_Parameters.TryGetValue(sName, value);

  return value;
}

void wdMaterialResource::SetTexture2DBinding(const wdHashedString& sName, const wdTexture2DResourceHandle& value)
{
  wdUInt32 uiIndex = wdInvalidIndex;
  for (wdUInt32 i = 0; i < m_mDesc.m_Texture2DBindings.GetCount(); ++i)
  {
    if (m_mDesc.m_Texture2DBindings[i].m_Name == sName)
    {
      uiIndex = i;
      break;
    }
  }

  if (value.IsValid())
  {
    if (uiIndex != wdInvalidIndex)
    {
      m_mDesc.m_Texture2DBindings[uiIndex].m_Value = value;
    }
    else
    {
      auto& binding = m_mDesc.m_Texture2DBindings.ExpandAndGetRef();
      binding.m_Name = sName;
      binding.m_Value = value;
    }
  }
  else
  {
    if (uiIndex != wdInvalidIndex)
    {
      m_mDesc.m_Texture2DBindings.RemoveAtAndSwap(uiIndex);
    }
  }

  m_iLastModified.Increment();

  m_ModifiedEvent.Broadcast(this);
}

void wdMaterialResource::SetTexture2DBinding(const char* szName, const wdTexture2DResourceHandle& value)
{
  wdTempHashedString sName(szName);

  wdUInt32 uiIndex = wdInvalidIndex;
  for (wdUInt32 i = 0; i < m_mDesc.m_Texture2DBindings.GetCount(); ++i)
  {
    if (m_mDesc.m_Texture2DBindings[i].m_Name == sName)
    {
      uiIndex = i;
      break;
    }
  }

  if (value.IsValid())
  {
    if (uiIndex != wdInvalidIndex)
    {
      m_mDesc.m_Texture2DBindings[uiIndex].m_Value = value;
    }
    else
    {
      auto& binding = m_mDesc.m_Texture2DBindings.ExpandAndGetRef();
      binding.m_Name.Assign(szName);
      binding.m_Value = value;
    }
  }
  else
  {
    if (uiIndex != wdInvalidIndex)
    {
      m_mDesc.m_Texture2DBindings.RemoveAtAndSwap(uiIndex);
    }
  }

  m_iLastModified.Increment();

  m_ModifiedEvent.Broadcast(this);
}

wdTexture2DResourceHandle wdMaterialResource::GetTexture2DBinding(const wdTempHashedString& sName)
{
  auto pCachedValues = GetOrUpdateCachedValues();

  // Use pointer to prevent ref counting
  wdTexture2DResourceHandle* pBinding;
  if (pCachedValues->m_Texture2DBindings.TryGetValue(sName, pBinding))
  {
    return *pBinding;
  }

  return wdTexture2DResourceHandle();
}


void wdMaterialResource::SetTextureCubeBinding(const wdHashedString& sName, const wdTextureCubeResourceHandle& value)
{
  wdUInt32 uiIndex = wdInvalidIndex;
  for (wdUInt32 i = 0; i < m_mDesc.m_TextureCubeBindings.GetCount(); ++i)
  {
    if (m_mDesc.m_TextureCubeBindings[i].m_Name == sName)
    {
      uiIndex = i;
      break;
    }
  }

  if (value.IsValid())
  {
    if (uiIndex != wdInvalidIndex)
    {
      m_mDesc.m_TextureCubeBindings[uiIndex].m_Value = value;
    }
    else
    {
      auto& binding = m_mDesc.m_TextureCubeBindings.ExpandAndGetRef();
      binding.m_Name = sName;
      binding.m_Value = value;
    }
  }
  else
  {
    if (uiIndex != wdInvalidIndex)
    {
      m_mDesc.m_TextureCubeBindings.RemoveAtAndSwap(uiIndex);
    }
  }

  m_iLastModified.Increment();

  m_ModifiedEvent.Broadcast(this);
}

void wdMaterialResource::SetTextureCubeBinding(const char* szName, const wdTextureCubeResourceHandle& value)
{
  wdTempHashedString sName(szName);

  wdUInt32 uiIndex = wdInvalidIndex;
  for (wdUInt32 i = 0; i < m_mDesc.m_TextureCubeBindings.GetCount(); ++i)
  {
    if (m_mDesc.m_TextureCubeBindings[i].m_Name == sName)
    {
      uiIndex = i;
      break;
    }
  }

  if (value.IsValid())
  {
    if (uiIndex != wdInvalidIndex)
    {
      m_mDesc.m_TextureCubeBindings[uiIndex].m_Value = value;
    }
    else
    {
      auto& binding = m_mDesc.m_TextureCubeBindings.ExpandAndGetRef();
      binding.m_Name.Assign(szName);
      binding.m_Value = value;
    }
  }
  else
  {
    if (uiIndex != wdInvalidIndex)
    {
      m_mDesc.m_TextureCubeBindings.RemoveAtAndSwap(uiIndex);
    }
  }

  m_iLastModified.Increment();

  m_ModifiedEvent.Broadcast(this);
}

wdTextureCubeResourceHandle wdMaterialResource::GetTextureCubeBinding(const wdTempHashedString& sName)
{
  auto pCachedValues = GetOrUpdateCachedValues();

  // Use pointer to prevent ref counting
  wdTextureCubeResourceHandle* pBinding;
  if (pCachedValues->m_TextureCubeBindings.TryGetValue(sName, pBinding))
  {
    return *pBinding;
  }

  return wdTextureCubeResourceHandle();
}

void wdMaterialResource::PreserveCurrentDesc()
{
  m_mOriginalDesc = m_mDesc;
}

void wdMaterialResource::ResetResource()
{
  if (m_mDesc != m_mOriginalDesc)
  {
    m_mDesc = m_mOriginalDesc;

    m_iLastModified.Increment();
    m_iLastConstantsModified.Increment();

    m_ModifiedEvent.Broadcast(this);
  }
}

const char* wdMaterialResource::GetDefaultMaterialFileName(DefaultMaterialType materialType)
{
  switch (materialType)
  {
    case DefaultMaterialType::Fullbright:
      return "Base/Materials/BaseMaterials/Fullbright.wdMaterialAsset";
    case DefaultMaterialType::FullbrightAlphaTest:
      return "Base/Materials/BaseMaterials/FullbrightAlphaTest.wdMaterialAsset";
    case DefaultMaterialType::Lit:
      return "Base/Materials/BaseMaterials/Lit.wdMaterialAsset";
    case DefaultMaterialType::LitAlphaTest:
      return "Base/Materials/BaseMaterials/LitAlphaTest.wdMaterialAsset";
    case DefaultMaterialType::Sky:
      return "Base/Materials/BaseMaterials/Sky.wdMaterialAsset";
    case DefaultMaterialType::MissingMaterial:
      return "Base/Materials/Common/MissingMaterial.wdMaterialAsset";
    default:
      WD_ASSERT_NOT_IMPLEMENTED;
      return "";
  }
}

wdResourceLoadDesc wdMaterialResource::UnloadData(Unload WhatToUnload)
{
  if (m_mDesc.m_hBaseMaterial.IsValid())
  {
    wdResourceLock<wdMaterialResource> pBaseMaterial(m_mDesc.m_hBaseMaterial, wdResourceAcquireMode::PointerOnly);

    auto d = wdMakeDelegate(&wdMaterialResource::OnBaseMaterialModified, this);
    if (pBaseMaterial->m_ModifiedEvent.HasEventHandler(d))
    {
      pBaseMaterial->m_ModifiedEvent.RemoveEventHandler(d);
    }
  }

  m_mDesc.Clear();
  m_mOriginalDesc.Clear();

  if (!m_hConstantBufferStorage.IsInvalidated())
  {
    wdRenderContext::DeleteConstantBufferStorage(m_hConstantBufferStorage);
    m_hConstantBufferStorage.Invalidate();
  }

  DeallocateCache(m_uiCacheIndex);
  m_uiCacheIndex = wdInvalidIndex;
  m_pCachedValues = nullptr;

  wdResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = wdResourceState::Unloaded;

  return res;
}

wdResourceLoadDesc wdMaterialResource::UpdateContent(wdStreamReader* pOuterStream)
{
  m_mDesc.Clear();
  m_mOriginalDesc.Clear();

  wdResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = wdResourceState::Loaded;

  if (pOuterStream == nullptr)
  {
    res.m_State = wdResourceState::LoadedResourceMissing;
    return res;
  }

  wdStringBuilder sAbsFilePath;
  (*pOuterStream) >> sAbsFilePath;

  if (sAbsFilePath.HasExtension("wdMaterialBin"))
  {
    wdStringBuilder sTemp, sTemp2;

    wdAssetFileHeader AssetHash;
    AssetHash.Read(*pOuterStream).IgnoreResult();

    wdUInt8 uiVersion = 0;
    (*pOuterStream) >> uiVersion;
    WD_ASSERT_DEV(uiVersion <= 6, "Unknown wdMaterialBin version {0}", uiVersion);

    wdUInt8 uiCompressionMode = 0;
    if (uiVersion >= 6)
    {
      *pOuterStream >> uiCompressionMode;
    }

    wdStreamReader* pInnerStream = pOuterStream;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
    wdCompressedStreamReaderZstd decompressorZstd;
#endif

    switch (uiCompressionMode)
    {
      case 0:
        break;

      case 1:
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
        decompressorZstd.SetInputStream(pOuterStream);
        pInnerStream = &decompressorZstd;
        break;
#else
        wdLog::Error("Material resource is compressed with zstandard, but support for this compressor is not compiled in.");
        res.m_State = wdResourceState::LoadedResourceMissing;
        return res;
#endif

      default:
        wdLog::Error("Material resource is compressed with an unknown algorithm.");
        res.m_State = wdResourceState::LoadedResourceMissing;
        return res;
    }

    wdStreamReader& s = *pInnerStream;

    // Base material
    {
      s >> sTemp;

      if (!sTemp.IsEmpty())
        m_mDesc.m_hBaseMaterial = wdResourceManager::LoadResource<wdMaterialResource>(sTemp);
    }

    if (uiVersion >= 4)
    {
      s >> sTemp;

      if (!sTemp.IsEmpty())
      {
        m_mDesc.m_sSurface.Assign(sTemp.GetData());
      }
    }

    // Shader
    {
      s >> sTemp;

      if (!sTemp.IsEmpty())
        m_mDesc.m_hShader = wdResourceManager::LoadResource<wdShaderResource>(sTemp);
    }

    // Permutation Variables
    {
      wdUInt16 uiPermVars;
      s >> uiPermVars;

      m_mDesc.m_PermutationVars.Reserve(uiPermVars);

      for (wdUInt16 i = 0; i < uiPermVars; ++i)
      {
        s >> sTemp;
        s >> sTemp2;

        if (!sTemp.IsEmpty() && !sTemp2.IsEmpty())
        {
          AddPermutationVar(sTemp, sTemp2);
        }
      }
    }

    // 2D Textures
    {
      wdUInt16 uiTextures = 0;
      s >> uiTextures;

      m_mDesc.m_Texture2DBindings.Reserve(uiTextures);

      for (wdUInt16 i = 0; i < uiTextures; ++i)
      {
        s >> sTemp;
        s >> sTemp2;

        if (!sTemp.IsEmpty() && !sTemp2.IsEmpty())
        {
          wdMaterialResourceDescriptor::Texture2DBinding& tc = m_mDesc.m_Texture2DBindings.ExpandAndGetRef();
          tc.m_Name.Assign(sTemp.GetData());
          tc.m_Value = wdResourceManager::LoadResource<wdTexture2DResource>(sTemp2);
        }
      }
    }

    // Cube Textures
    if (uiVersion >= 3)
    {
      wdUInt16 uiTextures = 0;
      s >> uiTextures;

      m_mDesc.m_TextureCubeBindings.Reserve(uiTextures);

      for (wdUInt16 i = 0; i < uiTextures; ++i)
      {
        s >> sTemp;
        s >> sTemp2;

        if (!sTemp.IsEmpty() && !sTemp2.IsEmpty())
        {
          wdMaterialResourceDescriptor::TextureCubeBinding& tc = m_mDesc.m_TextureCubeBindings.ExpandAndGetRef();
          tc.m_Name.Assign(sTemp.GetData());
          tc.m_Value = wdResourceManager::LoadResource<wdTextureCubeResource>(sTemp2);
        }
      }
    }


    if (uiVersion >= 2)
    {
      // Shader constants

      wdUInt16 uiConstants = 0;

      s >> uiConstants;

      m_mDesc.m_Parameters.Reserve(uiConstants);

      wdVariant vTemp;

      for (wdUInt16 i = 0; i < uiConstants; ++i)
      {
        s >> sTemp;
        s >> vTemp;

        if (!sTemp.IsEmpty() && vTemp.IsValid())
        {
          wdMaterialResourceDescriptor::Parameter& tc = m_mDesc.m_Parameters.ExpandAndGetRef();
          tc.m_Name.Assign(sTemp.GetData());
          tc.m_Value = vTemp;
        }
      }
    }

    if (uiVersion >= 5)
    {
      wdStreamReader& s = *pInnerStream;

      wdStringBuilder sResourceName;

      s >> sResourceName;

      wdTextureResourceLoader::LoadedData embedded;

      while (!sResourceName.IsEmpty())
      {
        wdUInt32 dataSize = 0;
        s >> dataSize;

        wdTextureResourceLoader::LoadTexFile(s, embedded).IgnoreResult();
        embedded.m_bIsFallback = true;

        wdDefaultMemoryStreamStorage storage;
        wdMemoryStreamWriter loadStreamWriter(&storage);
        wdTextureResourceLoader::WriteTextureLoadStream(loadStreamWriter, embedded);

        wdMemoryStreamReader loadStreamReader(&storage);

        wdTexture2DResourceHandle hTexture = wdResourceManager::LoadResource<wdTexture2DResource>(sResourceName);
        wdResourceManager::SetResourceLowResData(hTexture, &loadStreamReader);

        s >> sResourceName;
      }
    }
  }

  if (sAbsFilePath.HasExtension("wdMaterial"))
  {
    wdStringBuilder tmp, tmp2;
    wdOpenDdlReader reader;

    if (reader.ParseDocument(*pOuterStream, 0, wdLog::GetThreadLocalLogSystem()).Failed())
    {
      res.m_State = wdResourceState::LoadedResourceMissing;
      return res;
    }

    const wdOpenDdlReaderElement* pRoot = reader.GetRootElement();

    const wdOpenDdlReaderElement* pBase = pRoot->FindChildOfType(wdOpenDdlPrimitiveType::String, "BaseMaterial");
    const wdOpenDdlReaderElement* pshader = pRoot->FindChildOfType(wdOpenDdlPrimitiveType::String, "Shader");

    // Read the base material
    if (pBase)
    {
      tmp = pBase->GetPrimitivesString()[0];
      m_mDesc.m_hBaseMaterial = wdResourceManager::LoadResource<wdMaterialResource>(tmp);
    }

    // Read the shader
    if (pshader)
    {
      tmp = pshader->GetPrimitivesString()[0];
      m_mDesc.m_hShader = wdResourceManager::LoadResource<wdShaderResource>(tmp);
    }

    for (const wdOpenDdlReaderElement* pChild = pRoot->GetFirstChild(); pChild != nullptr; pChild = pChild->GetSibling())
    {
      // Read the shader permutation variables
      if (pChild->IsCustomType("Permutation"))
      {
        const wdOpenDdlReaderElement* pName = pChild->FindChildOfType(wdOpenDdlPrimitiveType::String, "Variable");
        const wdOpenDdlReaderElement* pValue = pChild->FindChildOfType(wdOpenDdlPrimitiveType::String, "Value");

        if (pName && pValue)
        {
          tmp = pName->GetPrimitivesString()[0];
          tmp2 = pValue->GetPrimitivesString()[0];

          AddPermutationVar(tmp, tmp2);
        }
      }

      // Read the shader constants
      if (pChild->IsCustomType("Constant"))
      {
        const wdOpenDdlReaderElement* pName = pChild->FindChildOfType(wdOpenDdlPrimitiveType::String, "Variable");
        const wdOpenDdlReaderElement* pValue = pChild->FindChild("Value");

        wdVariant value;
        if (pName && pValue && wdOpenDdlUtils::ConvertToVariant(pValue, value).Succeeded())
        {
          wdMaterialResourceDescriptor::Parameter& sc = m_mDesc.m_Parameters.ExpandAndGetRef();

          tmp = pName->GetPrimitivesString()[0];
          sc.m_Name.Assign(tmp.GetData());

          sc.m_Value = value;
        }
      }

      // Read the texture references
      if (pChild->IsCustomType("Texture2D"))
      {
        const wdOpenDdlReaderElement* pName = pChild->FindChildOfType(wdOpenDdlPrimitiveType::String, "Variable");
        const wdOpenDdlReaderElement* pValue = pChild->FindChildOfType(wdOpenDdlPrimitiveType::String, "Value");

        if (pName && pValue)
        {
          wdMaterialResourceDescriptor::Texture2DBinding& tc = m_mDesc.m_Texture2DBindings.ExpandAndGetRef();

          tmp = pName->GetPrimitivesString()[0];
          tc.m_Name.Assign(tmp.GetData());

          tmp = pValue->GetPrimitivesString()[0];
          tc.m_Value = wdResourceManager::LoadResource<wdTexture2DResource>(tmp);
        }
      }

      // Read the texture references
      if (pChild->IsCustomType("TextureCube"))
      {
        const wdOpenDdlReaderElement* pName = pChild->FindChildOfType(wdOpenDdlPrimitiveType::String, "Variable");
        const wdOpenDdlReaderElement* pValue = pChild->FindChildOfType(wdOpenDdlPrimitiveType::String, "Value");

        if (pName && pValue)
        {
          wdMaterialResourceDescriptor::TextureCubeBinding& tc = m_mDesc.m_TextureCubeBindings.ExpandAndGetRef();

          tmp = pName->GetPrimitivesString()[0];
          tc.m_Name.Assign(tmp.GetData());

          tmp = pValue->GetPrimitivesString()[0];
          tc.m_Value = wdResourceManager::LoadResource<wdTextureCubeResource>(tmp);
        }
      }
    }
  }

  if (m_mDesc.m_hBaseMaterial.IsValid())
  {
    // Block till the base material has been fully loaded to ensure that all parameters have their final value once this material is loaded.
    wdResourceLock<wdMaterialResource> pBaseMaterial(m_mDesc.m_hBaseMaterial, wdResourceAcquireMode::BlockTillLoaded);

    if (!pBaseMaterial->m_ModifiedEvent.HasEventHandler(wdMakeDelegate(&wdMaterialResource::OnBaseMaterialModified, this)))
    {
      pBaseMaterial->m_ModifiedEvent.AddEventHandler(wdMakeDelegate(&wdMaterialResource::OnBaseMaterialModified, this));
    }
  }

  m_mOriginalDesc = m_mDesc;

  m_iLastModified.Increment();
  m_iLastConstantsModified.Increment();

  m_ModifiedEvent.Broadcast(this);

  return res;
}

void wdMaterialResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU =
    sizeof(wdMaterialResource) + (wdUInt32)(m_mDesc.m_PermutationVars.GetHeapMemoryUsage() + m_mDesc.m_Parameters.GetHeapMemoryUsage() + m_mDesc.m_Texture2DBindings.GetHeapMemoryUsage() + m_mDesc.m_TextureCubeBindings.GetHeapMemoryUsage() + m_mOriginalDesc.m_PermutationVars.GetHeapMemoryUsage() +
                                            m_mOriginalDesc.m_Parameters.GetHeapMemoryUsage() + m_mOriginalDesc.m_Texture2DBindings.GetHeapMemoryUsage() + m_mOriginalDesc.m_TextureCubeBindings.GetHeapMemoryUsage());

  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

WD_RESOURCE_IMPLEMENT_CREATEABLE(wdMaterialResource, wdMaterialResourceDescriptor)
{
  m_mDesc = descriptor;
  m_mOriginalDesc = descriptor;

  wdResourceLoadDesc res;
  res.m_State = wdResourceState::Loaded;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (m_mDesc.m_hBaseMaterial.IsValid())
  {
    // Can't block here for the base material since this would result in a deadlock
    wdResourceLock<wdMaterialResource> pBaseMaterial(m_mDesc.m_hBaseMaterial, wdResourceAcquireMode::PointerOnly);
    pBaseMaterial->m_ModifiedEvent.AddEventHandler(wdMakeDelegate(&wdMaterialResource::OnBaseMaterialModified, this));
  }

  m_iLastModified.Increment();
  m_iLastConstantsModified.Increment();

  return res;
}

void wdMaterialResource::OnBaseMaterialModified(const wdMaterialResource* pModifiedMaterial)
{
  WD_ASSERT_DEV(m_mDesc.m_hBaseMaterial == pModifiedMaterial, "Implementation error");

  m_iLastModified.Increment();
  m_iLastConstantsModified.Increment();

  m_ModifiedEvent.Broadcast(this);
}

void wdMaterialResource::OnResourceEvent(const wdResourceEvent& resourceEvent)
{
  if (resourceEvent.m_Type != wdResourceEvent::Type::ResourceContentUpdated)
    return;

  if (m_pCachedValues != nullptr && m_pCachedValues->m_hShader == resourceEvent.m_pResource)
  {
    m_iLastConstantsModified.Increment();
  }
}

void wdMaterialResource::AddPermutationVar(const char* szName, const char* szValue)
{
  wdHashedString sName;
  sName.Assign(szName);
  wdHashedString sValue;
  sValue.Assign(szValue);

  if (wdShaderManager::IsPermutationValueAllowed(sName, sValue))
  {
    wdPermutationVar& pv = m_mDesc.m_PermutationVars.ExpandAndGetRef();
    pv.m_sName = sName;
    pv.m_sValue = sValue;
  }
}

bool wdMaterialResource::IsModified()
{
  return m_iLastModified != m_iLastUpdated;
}

bool wdMaterialResource::AreConstantsModified()
{
  return m_iLastConstantsModified != m_iLastConstantsUpdated;
}

void wdMaterialResource::UpdateConstantBuffer(wdShaderPermutationResource* pShaderPermutation)
{
  if (pShaderPermutation == nullptr)
    return;

  wdTempHashedString sConstantBufferName("wdMaterialConstants");
  const wdShaderResourceBinding* pBinding = pShaderPermutation->GetShaderStageBinary(wdGALShaderStage::PixelShader)->GetShaderResourceBinding(sConstantBufferName);
  if (pBinding == nullptr)
  {
    pBinding = pShaderPermutation->GetShaderStageBinary(wdGALShaderStage::VertexShader)->GetShaderResourceBinding(sConstantBufferName);
  }

  const wdShaderConstantBufferLayout* pLayout = pBinding != nullptr ? pBinding->m_pLayout : nullptr;
  if (pLayout == nullptr)
    return;

  auto pCachedValues = GetOrUpdateCachedValues();

  m_iLastConstantsUpdated = m_iLastConstantsModified;

  if (m_hConstantBufferStorage.IsInvalidated())
  {
    m_hConstantBufferStorage = wdRenderContext::CreateConstantBufferStorage(pLayout->m_uiTotalSize);
  }

  wdConstantBufferStorageBase* pStorage = nullptr;
  if (wdRenderContext::TryGetConstantBufferStorage(m_hConstantBufferStorage, pStorage))
  {
    wdArrayPtr<wdUInt8> data = pStorage->GetRawDataForWriting();
    if (data.GetCount() != pLayout->m_uiTotalSize)
    {
      wdRenderContext::DeleteConstantBufferStorage(m_hConstantBufferStorage);
      m_hConstantBufferStorage = wdRenderContext::CreateConstantBufferStorage(pLayout->m_uiTotalSize);

      WD_VERIFY(wdRenderContext::TryGetConstantBufferStorage(m_hConstantBufferStorage, pStorage), "");
    }

    for (auto& constant : pLayout->m_Constants)
    {
      if (constant.m_uiOffset + wdShaderConstantBufferLayout::Constant::s_TypeSize[constant.m_Type.GetValue()] <= data.GetCount())
      {
        wdUInt8* pDest = &data[constant.m_uiOffset];

        wdVariant* pValue = nullptr;
        pCachedValues->m_Parameters.TryGetValue(constant.m_sName, pValue);

        constant.CopyDataFormVariant(pDest, pValue);
      }
    }
  }
}

wdMaterialResource::CachedValues* wdMaterialResource::GetOrUpdateCachedValues()
{
  if (!IsModified())
  {
    WD_ASSERT_DEV(m_pCachedValues != nullptr, "");
    return m_pCachedValues;
  }

  wdHybridArray<wdMaterialResource*, 16> materialHierarchy;
  wdMaterialResource* pCurrentMaterial = this;

  while (true)
  {
    materialHierarchy.PushBack(pCurrentMaterial);

    const wdMaterialResourceHandle& hBaseMaterial = pCurrentMaterial->m_mDesc.m_hBaseMaterial;
    if (!hBaseMaterial.IsValid())
      break;

    // Ensure that the base material is loaded at this point.
    // For loaded materials this will always be the case but is still necessary for runtime created materials.
    pCurrentMaterial = wdResourceManager::BeginAcquireResource(hBaseMaterial, wdResourceAcquireMode::BlockTillLoaded);
  }

  WD_SCOPE_EXIT(for (wdUInt32 i = materialHierarchy.GetCount(); i-- > 1;) {
    wdMaterialResource* pMaterial = materialHierarchy[i];
    wdResourceManager::EndAcquireResource(pMaterial);

    materialHierarchy[i] = nullptr;
  });

  WD_LOCK(m_UpdateCacheMutex);

  if (!IsModified())
  {
    WD_ASSERT_DEV(m_pCachedValues != nullptr, "");
    return m_pCachedValues;
  }

  m_pCachedValues = AllocateCache();

  // set state of parent material first
  for (wdUInt32 i = materialHierarchy.GetCount(); i-- > 0;)
  {
    wdMaterialResource* pMaterial = materialHierarchy[i];
    const wdMaterialResourceDescriptor& desc = pMaterial->m_mDesc;

    if (desc.m_hShader.IsValid())
      m_pCachedValues->m_hShader = desc.m_hShader;

    for (const auto& permutationVar : desc.m_PermutationVars)
    {
      m_pCachedValues->m_PermutationVars.Insert(permutationVar.m_sName, permutationVar.m_sValue);
    }

    for (const auto& param : desc.m_Parameters)
    {
      m_pCachedValues->m_Parameters.Insert(param.m_Name, param.m_Value);
    }

    for (const auto& textureBinding : desc.m_Texture2DBindings)
    {
      m_pCachedValues->m_Texture2DBindings.Insert(textureBinding.m_Name, textureBinding.m_Value);
    }

    for (const auto& textureBinding : desc.m_TextureCubeBindings)
    {
      m_pCachedValues->m_TextureCubeBindings.Insert(textureBinding.m_Name, textureBinding.m_Value);
    }
  }

  m_iLastUpdated = m_iLastModified;
  return m_pCachedValues;
}

namespace
{
  static wdMutex s_MaterialCacheMutex;

  struct FreeCacheEntry
  {
    WD_DECLARE_POD_TYPE();

    wdUInt32 m_uiIndex;
    wdUInt64 m_uiFrame;
  };

  static wdDynamicArray<FreeCacheEntry, wdStaticAllocatorWrapper> s_FreeMaterialCacheEntries;
} // namespace

wdMaterialResource::CachedValues* wdMaterialResource::AllocateCache()
{
  WD_LOCK(s_MaterialCacheMutex);

  wdUInt32 uiOldCacheIndex = m_uiCacheIndex;

  wdUInt64 uiCurrentFrame = wdRenderWorld::GetFrameCounter();
  if (!s_FreeMaterialCacheEntries.IsEmpty() && s_FreeMaterialCacheEntries[0].m_uiFrame < uiCurrentFrame)
  {
    m_uiCacheIndex = s_FreeMaterialCacheEntries[0].m_uiIndex;
    s_FreeMaterialCacheEntries.RemoveAtAndCopy(0);
  }
  else
  {
    m_uiCacheIndex = s_CachedValues.GetCount();
    s_CachedValues.ExpandAndGetRef();
  }

  DeallocateCache(uiOldCacheIndex);

  return &s_CachedValues[m_uiCacheIndex];
}

void wdMaterialResource::DeallocateCache(wdUInt32 uiCacheIndex)
{
  if (uiCacheIndex != wdInvalidIndex)
  {
    WD_LOCK(s_MaterialCacheMutex);

    if (uiCacheIndex < s_CachedValues.GetCount())
    {
      auto& freeEntry = s_FreeMaterialCacheEntries.ExpandAndGetRef();
      freeEntry.m_uiIndex = uiCacheIndex;
      freeEntry.m_uiFrame = wdRenderWorld::GetFrameCounter();
    }
  }
}

// static
void wdMaterialResource::ClearCache()
{
  WD_LOCK(s_MaterialCacheMutex);

  s_CachedValues.Clear();
  s_FreeMaterialCacheEntries.Clear();
}

const wdMaterialResourceDescriptor& wdMaterialResource::GetCurrentDesc() const
{
  return m_mDesc;
}

WD_STATICLINK_FILE(RendererCore, RendererCore_Material_Implementation_MaterialResource);
