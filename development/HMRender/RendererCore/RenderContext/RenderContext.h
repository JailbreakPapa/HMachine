#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Strings/String.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/ViewData.h>
#include <RendererCore/RenderContext/Implementation/RenderContextStructs.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderStageBinary.h>
#include <RendererCore/ShaderCompiler/PermutationGenerator.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/Texture3DResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/Pass.h>
#include <RendererFoundation/Shader/Shader.h>
#include <RendererFoundation/Shader/ShaderUtils.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/GlobalConstants.h>

struct wdRenderWorldRenderEvent;

//////////////////////////////////////////////////////////////////////////
// wdRenderContext
//////////////////////////////////////////////////////////////////////////

class WD_RENDERERCORE_DLL wdRenderContext
{
private:
  wdRenderContext();
  ~wdRenderContext();
  friend class wdMemoryUtils;

  static wdRenderContext* s_pDefaultInstance;
  static wdHybridArray<wdRenderContext*, 4> s_Instances;

public:
  static wdRenderContext* GetDefaultInstance();
  static wdRenderContext* CreateInstance();
  static void DestroyInstance(wdRenderContext* pRenderer);

public:
  struct Statistics
  {
    Statistics();
    void Reset();

    wdUInt32 m_uiFailedDrawcalls;
  };

  Statistics GetAndResetStatistics();

  wdGALRenderCommandEncoder* BeginRendering(wdGALPass* pGALPass, const wdGALRenderingSetup& renderingSetup, const wdRectFloat& viewport, const char* szName = "", bool bStereoRendering = false);
  void EndRendering();

  wdGALComputeCommandEncoder* BeginCompute(wdGALPass* pGALPass, const char* szName = "");
  void EndCompute();

  // Helper class to automatically end rendering or compute on scope exit
  template <typename T>
  class CommandEncoderScope
  {
    WD_DISALLOW_COPY_AND_ASSIGN(CommandEncoderScope);

  public:
    WD_ALWAYS_INLINE ~CommandEncoderScope()
    {
      m_RenderContext.EndCommandEncoder(m_pGALCommandEncoder);

      if (m_pGALPass != nullptr)
      {
        wdGALDevice::GetDefaultDevice()->EndPass(m_pGALPass);
      }
    }

    WD_ALWAYS_INLINE T* operator->() { return m_pGALCommandEncoder; }
    WD_ALWAYS_INLINE operator const T*() { return m_pGALCommandEncoder; }

  private:
    friend class wdRenderContext;

    WD_ALWAYS_INLINE CommandEncoderScope(wdRenderContext& renderContext, wdGALPass* pGALPass, T* pGALCommandEncoder)
      : m_RenderContext(renderContext)
      , m_pGALPass(pGALPass)
      , m_pGALCommandEncoder(pGALCommandEncoder)
    {
    }

    wdRenderContext& m_RenderContext;
    wdGALPass* m_pGALPass;
    T* m_pGALCommandEncoder;
  };

  using RenderingScope = CommandEncoderScope<wdGALRenderCommandEncoder>;
  WD_ALWAYS_INLINE static RenderingScope BeginRenderingScope(wdGALPass* pGALPass, const wdRenderViewContext& viewContext, const wdGALRenderingSetup& renderingSetup, const char* szName = "", bool bStereoRendering = false)
  {
    return RenderingScope(*viewContext.m_pRenderContext, nullptr, viewContext.m_pRenderContext->BeginRendering(pGALPass, renderingSetup, viewContext.m_pViewData->m_ViewPortRect, szName, bStereoRendering));
  }

  WD_ALWAYS_INLINE static RenderingScope BeginPassAndRenderingScope(const wdRenderViewContext& viewContext, const wdGALRenderingSetup& renderingSetup, const char* szName, bool bStereoRendering = false)
  {
    wdGALPass* pGALPass = wdGALDevice::GetDefaultDevice()->BeginPass(szName);

    return RenderingScope(*viewContext.m_pRenderContext, pGALPass, viewContext.m_pRenderContext->BeginRendering(pGALPass, renderingSetup, viewContext.m_pViewData->m_ViewPortRect, "", bStereoRendering));
  }

  using ComputeScope = CommandEncoderScope<wdGALComputeCommandEncoder>;
  WD_ALWAYS_INLINE static ComputeScope BeginComputeScope(wdGALPass* pGALPass, const wdRenderViewContext& viewContext, const char* szName = "")
  {
    return ComputeScope(*viewContext.m_pRenderContext, nullptr, viewContext.m_pRenderContext->BeginCompute(pGALPass, szName));
  }

  WD_ALWAYS_INLINE static ComputeScope BeginPassAndComputeScope(const wdRenderViewContext& viewContext, const char* szName)
  {
    wdGALPass* pGALPass = wdGALDevice::GetDefaultDevice()->BeginPass(szName);

    return ComputeScope(*viewContext.m_pRenderContext, pGALPass, viewContext.m_pRenderContext->BeginCompute(pGALPass));
  }

  WD_ALWAYS_INLINE wdGALCommandEncoder* GetCommandEncoder()
  {
    WD_ASSERT_DEBUG(m_pGALCommandEncoder != nullptr, "BeginRendering/Compute has not been called");
    return m_pGALCommandEncoder;
  }

  WD_ALWAYS_INLINE wdGALRenderCommandEncoder* GetRenderCommandEncoder()
  {
    WD_ASSERT_DEBUG(m_pGALCommandEncoder != nullptr && !m_bCompute, "BeginRendering has not been called");
    return static_cast<wdGALRenderCommandEncoder*>(m_pGALCommandEncoder);
  }

  WD_ALWAYS_INLINE wdGALComputeCommandEncoder* GetComputeCommandEncoder()
  {
    WD_ASSERT_DEBUG(m_pGALCommandEncoder != nullptr && m_bCompute, "BeginCompute has not been called");
    return static_cast<wdGALComputeCommandEncoder*>(m_pGALCommandEncoder);
  }


  // Member Functions
  void SetShaderPermutationVariable(const char* szName, const wdTempHashedString& sValue);
  void SetShaderPermutationVariable(const wdHashedString& sName, const wdHashedString& sValue);

  void BindMaterial(const wdMaterialResourceHandle& hMaterial);

  void BindTexture2D(const wdTempHashedString& sSlotName, const wdTexture2DResourceHandle& hTexture, wdResourceAcquireMode acquireMode = wdResourceAcquireMode::AllowLoadingFallback);
  void BindTexture3D(const wdTempHashedString& sSlotName, const wdTexture3DResourceHandle& hTexture, wdResourceAcquireMode acquireMode = wdResourceAcquireMode::AllowLoadingFallback);
  void BindTextureCube(const wdTempHashedString& sSlotName, const wdTextureCubeResourceHandle& hTexture, wdResourceAcquireMode acquireMode = wdResourceAcquireMode::AllowLoadingFallback);

  void BindTexture2D(const wdTempHashedString& sSlotName, wdGALResourceViewHandle hResourceView);
  void BindTexture3D(const wdTempHashedString& sSlotName, wdGALResourceViewHandle hResourceView);
  void BindTextureCube(const wdTempHashedString& sSlotName, wdGALResourceViewHandle hResourceView);

  /// Binds a read+write texture or buffer
  void BindUAV(const wdTempHashedString& sSlotName, wdGALUnorderedAccessViewHandle hUnorderedAccessViewHandle);

  void BindSamplerState(const wdTempHashedString& sSlotName, wdGALSamplerStateHandle hSamplerSate);

  void BindBuffer(const wdTempHashedString& sSlotName, wdGALResourceViewHandle hResourceView);

  void BindConstantBuffer(const wdTempHashedString& sSlotName, wdGALBufferHandle hConstantBuffer);
  void BindConstantBuffer(const wdTempHashedString& sSlotName, wdConstantBufferStorageHandle hConstantBufferStorage);

  /// \brief Sets the currently active shader on the given render context.
  ///
  /// This function has no effect until the next draw or dispatch call on the context.
  void BindShader(const wdShaderResourceHandle& hShader, wdBitflags<wdShaderBindFlags> flags = wdShaderBindFlags::Default);

  void BindMeshBuffer(const wdDynamicMeshBufferResourceHandle& hDynamicMeshBuffer);
  void BindMeshBuffer(const wdMeshBufferResourceHandle& hMeshBuffer);
  void BindMeshBuffer(wdGALBufferHandle hVertexBuffer, wdGALBufferHandle hIndexBuffer, const wdVertexDeclarationInfo* pVertexDeclarationInfo, wdGALPrimitiveTopology::Enum topology, wdUInt32 uiPrimitiveCount, wdGALBufferHandle hVertexBuffer2 = {}, wdGALBufferHandle hVertexBuffer3 = {}, wdGALBufferHandle hVertexBuffer4 = {});
  WD_ALWAYS_INLINE void BindNullMeshBuffer(wdGALPrimitiveTopology::Enum topology, wdUInt32 uiPrimitiveCount)
  {
    BindMeshBuffer(wdGALBufferHandle(), wdGALBufferHandle(), nullptr, topology, uiPrimitiveCount);
  }

  wdResult DrawMeshBuffer(wdUInt32 uiPrimitiveCount = 0xFFFFFFFF, wdUInt32 uiFirstPrimitive = 0, wdUInt32 uiInstanceCount = 1);

  wdResult Dispatch(wdUInt32 uiThreadGroupCountX, wdUInt32 uiThreadGroupCountY = 1, wdUInt32 uiThreadGroupCountZ = 1);

  wdResult ApplyContextStates(bool bForce = false);
  void ResetContextState();

  wdGlobalConstants& WriteGlobalConstants();
  const wdGlobalConstants& ReadGlobalConstants() const;

  /// \brief Sets the texture filter mode that is used by default for texture resources.
  ///
  /// The built in default is Anisotropic 4x.
  /// If the default setting is changed, already loaded textures might not adjust.
  /// Nearest filtering is not allowed as a default filter.
  void SetDefaultTextureFilter(wdTextureFilterSetting::Enum filter);

  /// \brief Returns the texture filter mode that is used by default for textures.
  wdTextureFilterSetting::Enum GetDefaultTextureFilter() const { return m_DefaultTextureFilter; }

  /// \brief Returns the 'fixed' texture filter setting that the combination of default texture filter and given \a configuration defines.
  ///
  /// If \a configuration is set to a fixed filter, that setting is returned.
  /// If it is one of LowestQuality to HighestQuality, the adjusted default filter is returned.
  /// When the default filter is used (with adjustments), the allowed range is Bilinear to Aniso16x, the Nearest filter is never used.
  wdTextureFilterSetting::Enum GetSpecificTextureFilter(wdTextureFilterSetting::Enum configuration) const;

  /// \brief Set async shader loading. During runtime all shaders should be preloaded so this is off by default.
  void SetAllowAsyncShaderLoading(bool bAllow);

  /// \brief Returns async shader loading. During runtime all shaders should be preloaded so this is off by default.
  bool GetAllowAsyncShaderLoading();


  // Static Functions
public:
  // Constant buffer storage handling
  template <typename T>
  WD_ALWAYS_INLINE static wdConstantBufferStorageHandle CreateConstantBufferStorage()
  {
    return CreateConstantBufferStorage(sizeof(T));
  }

  template <typename T>
  WD_FORCE_INLINE static wdConstantBufferStorageHandle CreateConstantBufferStorage(wdConstantBufferStorage<T>*& out_pStorage)
  {
    wdConstantBufferStorageBase* pStorage;
    wdConstantBufferStorageHandle hStorage = CreateConstantBufferStorage(sizeof(T), pStorage);
    out_pStorage = static_cast<wdConstantBufferStorage<T>*>(pStorage);
    return hStorage;
  }

  WD_FORCE_INLINE static wdConstantBufferStorageHandle CreateConstantBufferStorage(wdUInt32 uiSizeInBytes)
  {
    wdConstantBufferStorageBase* pStorage;
    return CreateConstantBufferStorage(uiSizeInBytes, pStorage);
  }

  static wdConstantBufferStorageHandle CreateConstantBufferStorage(wdUInt32 uiSizeInBytes, wdConstantBufferStorageBase*& out_pStorage);
  static void DeleteConstantBufferStorage(wdConstantBufferStorageHandle hStorage);

  template <typename T>
  WD_FORCE_INLINE static bool TryGetConstantBufferStorage(wdConstantBufferStorageHandle hStorage, wdConstantBufferStorage<T>*& out_pStorage)
  {
    wdConstantBufferStorageBase* pStorage = nullptr;
    bool bResult = TryGetConstantBufferStorage(hStorage, pStorage);
    out_pStorage = static_cast<wdConstantBufferStorage<T>*>(pStorage);
    return bResult;
  }

  static bool TryGetConstantBufferStorage(wdConstantBufferStorageHandle hStorage, wdConstantBufferStorageBase*& out_pStorage);

  template <typename T>
  WD_FORCE_INLINE static T* GetConstantBufferData(wdConstantBufferStorageHandle hStorage)
  {
    wdConstantBufferStorage<T>* pStorage = nullptr;
    if (TryGetConstantBufferStorage(hStorage, pStorage))
    {
      return &(pStorage->GetDataForWriting());
    }

    return nullptr;
  }

  // Default sampler state
  static wdGALSamplerStateHandle GetDefaultSamplerState(wdBitflags<wdDefaultSamplerFlags> flags);

private:
  WD_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererCore, RendererContext);

  static void LoadBuiltinShader(wdShaderUtils::wdBuiltinShaderType type, wdShaderUtils::wdBuiltinShader& out_shader);
  static void OnEngineShutdown();

private:
  Statistics m_Statistics;
  wdBitflags<wdRenderContextFlags> m_StateFlags;
  wdShaderResourceHandle m_hActiveShader;
  wdGALShaderHandle m_hActiveGALShader;

  wdHashTable<wdHashedString, wdHashedString> m_PermutationVariables;
  wdMaterialResourceHandle m_hNewMaterial;
  wdMaterialResourceHandle m_hMaterial;

  wdShaderPermutationResourceHandle m_hActiveShaderPermutation;

  wdBitflags<wdShaderBindFlags> m_ShaderBindFlags;

  wdGALBufferHandle m_hVertexBuffers[4];
  wdGALBufferHandle m_hIndexBuffer;
  const wdVertexDeclarationInfo* m_pVertexDeclarationInfo;
  wdGALPrimitiveTopology::Enum m_Topology;
  wdUInt32 m_uiMeshBufferPrimitiveCount;
  wdEnum<wdTextureFilterSetting> m_DefaultTextureFilter;
  bool m_bAllowAsyncShaderLoading;
  bool m_bStereoRendering = false;

  wdHashTable<wdUInt64, wdGALResourceViewHandle> m_BoundTextures2D;
  wdHashTable<wdUInt64, wdGALResourceViewHandle> m_BoundTextures3D;
  wdHashTable<wdUInt64, wdGALResourceViewHandle> m_BoundTexturesCube;
  wdHashTable<wdUInt64, wdGALUnorderedAccessViewHandle> m_BoundUAVs;
  wdHashTable<wdUInt64, wdGALSamplerStateHandle> m_BoundSamplers;
  wdHashTable<wdUInt64, wdGALResourceViewHandle> m_BoundBuffer;

  struct BoundConstantBuffer
  {
    WD_DECLARE_POD_TYPE();

    BoundConstantBuffer() = default;
    BoundConstantBuffer(wdGALBufferHandle hConstantBuffer)
      : m_hConstantBuffer(hConstantBuffer)
    {
    }
    BoundConstantBuffer(wdConstantBufferStorageHandle hConstantBufferStorage)
      : m_hConstantBufferStorage(hConstantBufferStorage)
    {
    }

    wdGALBufferHandle m_hConstantBuffer;
    wdConstantBufferStorageHandle m_hConstantBufferStorage;
  };

  wdHashTable<wdUInt64, BoundConstantBuffer> m_BoundConstantBuffers;

  wdConstantBufferStorageHandle m_hGlobalConstantBufferStorage;

  struct ShaderVertexDecl
  {
    wdGALShaderHandle m_hShader;
    wdUInt32 m_uiVertexDeclarationHash;

    WD_FORCE_INLINE bool operator<(const ShaderVertexDecl& rhs) const
    {
      if (m_hShader < rhs.m_hShader)
        return true;
      if (rhs.m_hShader < m_hShader)
        return false;
      return m_uiVertexDeclarationHash < rhs.m_uiVertexDeclarationHash;
    }

    WD_FORCE_INLINE bool operator==(const ShaderVertexDecl& rhs) const
    {
      return (m_hShader == rhs.m_hShader && m_uiVertexDeclarationHash == rhs.m_uiVertexDeclarationHash);
    }
  };

  static wdResult BuildVertexDeclaration(wdGALShaderHandle hShader, const wdVertexDeclarationInfo& decl, wdGALVertexDeclarationHandle& out_Declaration);

  static wdMap<ShaderVertexDecl, wdGALVertexDeclarationHandle> s_GALVertexDeclarations;

  static wdMutex s_ConstantBufferStorageMutex;
  static wdIdTable<wdConstantBufferStorageId, wdConstantBufferStorageBase*> s_ConstantBufferStorageTable;
  static wdMap<wdUInt32, wdDynamicArray<wdConstantBufferStorageBase*>> s_FreeConstantBufferStorage;

  static wdGALSamplerStateHandle s_hDefaultSamplerStates[4];

private: // Per Renderer States
  friend RenderingScope;
  friend ComputeScope;
  WD_ALWAYS_INLINE void EndCommandEncoder(wdGALRenderCommandEncoder*) { EndRendering(); }
  WD_ALWAYS_INLINE void EndCommandEncoder(wdGALComputeCommandEncoder*) { EndCompute(); }

  wdGALPass* m_pGALPass = nullptr;
  wdGALCommandEncoder* m_pGALCommandEncoder = nullptr;
  bool m_bCompute = false;

  // Member Functions
  void UploadConstants();

  void SetShaderPermutationVariableInternal(const wdHashedString& sName, const wdHashedString& sValue);
  void BindShaderInternal(const wdShaderResourceHandle& hShader, wdBitflags<wdShaderBindFlags> flags);
  wdShaderPermutationResource* ApplyShaderState();
  wdMaterialResource* ApplyMaterialState();
  void ApplyConstantBufferBindings(const wdShaderStageBinary* pBinary);
  void ApplyTextureBindings(wdGALShaderStage::Enum stage, const wdShaderStageBinary* pBinary);
  void ApplyUAVBindings(const wdShaderStageBinary* pBinary);
  void ApplySamplerBindings(wdGALShaderStage::Enum stage, const wdShaderStageBinary* pBinary);
  void ApplyBufferBindings(wdGALShaderStage::Enum stage, const wdShaderStageBinary* pBinary);
};
