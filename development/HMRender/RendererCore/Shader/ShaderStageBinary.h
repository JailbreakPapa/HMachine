#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Enum.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

class WD_RENDERERCORE_DLL wdShaderConstantBufferLayout : public wdRefCounted
{
public:
  struct Constant
  {
    WD_DECLARE_MEM_RELOCATABLE_TYPE();

    struct Type
    {
      using StorageType = wdUInt8;

      enum Enum
      {
        Default,
        Float1,
        Float2,
        Float3,
        Float4,
        Int1,
        Int2,
        Int3,
        Int4,
        UInt1,
        UInt2,
        UInt3,
        UInt4,
        Mat3x3,
        Mat4x4,
        Transform,
        Bool,
        Struct,
        ENUM_COUNT
      };
    };

    static wdUInt32 s_TypeSize[Type::ENUM_COUNT];

    Constant()
    {
      m_uiArrayElements = 0;
      m_uiOffset = 0;
    }

    void CopyDataFormVariant(wdUInt8* pDest, wdVariant* pValue) const;

    wdHashedString m_sName;
    wdEnum<Type> m_Type;
    wdUInt8 m_uiArrayElements;
    wdUInt16 m_uiOffset;
  };

private:
  friend class wdShaderStageBinary;
  friend class wdMemoryUtils;

  wdShaderConstantBufferLayout();
  ~wdShaderConstantBufferLayout();

public:
  wdResult Write(wdStreamWriter& inout_stream) const;
  wdResult Read(wdStreamReader& inout_stream);

  wdUInt32 m_uiTotalSize;
  wdHybridArray<Constant, 16> m_Constants;
};

struct WD_RENDERERCORE_DLL wdShaderResourceBinding
{
  WD_DECLARE_MEM_RELOCATABLE_TYPE();


  wdShaderResourceBinding();
  ~wdShaderResourceBinding();

  wdShaderResourceType::Enum m_Type;
  wdInt32 m_iSlot;
  wdHashedString m_sName;
  wdScopedRefPointer<wdShaderConstantBufferLayout> m_pLayout;
};

class WD_RENDERERCORE_DLL wdShaderStageBinary
{
public:
  enum Version
  {
    Version0,
    Version1,
    Version2,
    Version3, // Added Material Parameters
    Version4, // Constant buffer layouts
    Version5, // Debug flag

    ENUM_COUNT,
    VersionCurrent = ENUM_COUNT - 1
  };

  wdShaderStageBinary();
  ~wdShaderStageBinary();

  wdResult Write(wdStreamWriter& inout_stream) const;
  wdResult Read(wdStreamReader& inout_stream);

  wdDynamicArray<wdUInt8>& GetByteCode();

  void AddShaderResourceBinding(const wdShaderResourceBinding& binding);
  wdArrayPtr<const wdShaderResourceBinding> GetShaderResourceBindings() const;
  const wdShaderResourceBinding* GetShaderResourceBinding(const wdTempHashedString& sName) const;

  wdShaderConstantBufferLayout* CreateConstantBufferLayout() const;

private:
  friend class wdRenderContext;
  friend class wdShaderCompiler;
  friend class wdShaderPermutationResource;
  friend class wdShaderPermutationResourceLoader;

  wdUInt32 m_uiSourceHash = 0;
  wdGALShaderStage::Enum m_Stage = wdGALShaderStage::ENUM_COUNT;
  wdDynamicArray<wdUInt8> m_ByteCode;
  wdScopedRefPointer<wdGALShaderByteCode> m_GALByteCode;
  wdHybridArray<wdShaderResourceBinding, 8> m_ShaderResourceBindings;
  bool m_bWasCompiledWithDebug = false;

  wdResult WriteStageBinary(wdLogInterface* pLog) const;
  static wdShaderStageBinary* LoadStageBinary(wdGALShaderStage::Enum Stage, wdUInt32 uiHash);

  static void OnEngineShutdown();

  static wdMap<wdUInt32, wdShaderStageBinary> s_ShaderStageBinaries[wdGALShaderStage::ENUM_COUNT];
};
