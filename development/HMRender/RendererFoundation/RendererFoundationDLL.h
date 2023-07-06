#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Id.h>
#include <Foundation/Types/RefCounted.h>

// Configure the DLL Import/Export Define
#if WD_ENABLED(WD_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_RENDERERFOUNDATION_LIB
#    define WD_RENDERERFOUNDATION_DLL WD_DECL_EXPORT
#  else
#    define WD_RENDERERFOUNDATION_DLL WD_DECL_IMPORT
#  endif
#else
#  define WD_RENDERERFOUNDATION_DLL
#endif

// Necessary array sizes
#define WD_GAL_MAX_CONSTANT_BUFFER_COUNT 16
#define WD_GAL_MAX_SAMPLER_COUNT 16
#define WD_GAL_MAX_VERTEX_BUFFER_COUNT 16
#define WD_GAL_MAX_RENDERTARGET_COUNT 8

// Forward declarations

struct wdGALDeviceCreationDescription;
struct wdGALSwapChainCreationDescription;
struct wdGALWindowSwapChainCreationDescription;
struct wdGALShaderCreationDescription;
struct wdGALTextureCreationDescription;
struct wdGALBufferCreationDescription;
struct wdGALDepthStencilStateCreationDescription;
struct wdGALBlendStateCreationDescription;
struct wdGALRasterizerStateCreationDescription;
struct wdGALVertexDeclarationCreationDescription;
struct wdGALQueryCreationDescription;
struct wdGALSamplerStateCreationDescription;
struct wdGALResourceViewCreationDescription;
struct wdGALRenderTargetViewCreationDescription;
struct wdGALUnorderedAccessViewCreationDescription;

class wdGALSwapChain;
class wdGALShader;
class wdGALResourceBase;
class wdGALTexture;
class wdGALBuffer;
class wdGALDepthStencilState;
class wdGALBlendState;
class wdGALRasterizerState;
class wdGALRenderTargetSetup;
class wdGALVertexDeclaration;
class wdGALQuery;
class wdGALSamplerState;
class wdGALResourceView;
class wdGALRenderTargetView;
class wdGALUnorderedAccessView;
class wdGALDevice;
class wdGALPass;
class wdGALCommandEncoder;
class wdGALRenderCommandEncoder;
class wdGALComputeCommandEncoder;

// Basic enums
struct wdGALPrimitiveTopology
{
  typedef wdUInt8 StorageType;
  enum Enum
  {
    // keep this order, it is used to allocate the desired number of indices in wdMeshBufferResourceDescriptor::AllocateStreams
    Points,    // 1 index per primitive
    Lines,     // 2 indices per primitive
    Triangles, // 3 indices per primitive
    ENUM_COUNT,
    Default = Triangles
  };

  static wdUInt32 VerticesPerPrimitive(wdGALPrimitiveTopology::Enum e) { return (wdUInt32)e + 1; }
};

struct WD_RENDERERFOUNDATION_DLL wdGALIndexType
{
  enum Enum
  {
    None,   // indices are not used, vertices are just used in order to form primitives
    UShort, // 16 bit indices are used to select which vertices shall form a primitive, thus meshes can only use up to 65535 vertices
    UInt,   // 32 bit indices are used to select which vertices shall form a primitive

    ENUM_COUNT
  };


  /// \brief The size in bytes of a single element of the given index format.
  static wdUInt8 GetSize(wdGALIndexType::Enum format) { return s_Size[format]; }

private:
  static const wdUInt8 s_Size[wdGALIndexType::ENUM_COUNT];
};


struct WD_RENDERERFOUNDATION_DLL wdGALShaderStage
{
  enum Enum : wdUInt8
  {
    VertexShader,
    HullShader,
    DomainShader,
    GeometryShader,
    PixelShader,

    ComputeShader,

    ENUM_COUNT
  };

  static const char* Names[ENUM_COUNT];
};

struct WD_RENDERERFOUNDATION_DLL wdGALMSAASampleCount
{
  typedef wdUInt8 StorageType;

  enum Enum
  {
    None = 1,
    TwoSamples = 2,
    FourSamples = 4,
    EightSamples = 8,

    ENUM_COUNT = 4,

    Default = None
  };
};

WD_DECLARE_REFLECTABLE_TYPE(WD_RENDERERFOUNDATION_DLL, wdGALMSAASampleCount);

struct wdGALTextureType
{
  typedef wdUInt8 StorageType;

  enum Enum
  {
    Invalid = -1,
    Texture2D = 0,
    TextureCube,
    Texture3D,
    Texture2DProxy,

    ENUM_COUNT,

    Default = Texture2D
  };
};

struct wdGALBlend
{
  enum Enum
  {
    Zero = 0,
    One,
    SrcColor,
    InvSrcColor,
    SrcAlpha,
    InvSrcAlpha,
    DestAlpha,
    InvDestAlpha,
    DestColor,
    InvDestColor,
    SrcAlphaSaturated,
    BlendFactor,
    InvBlendFactor,

    ENUM_COUNT
  };
};

struct wdGALBlendOp
{
  enum Enum
  {
    Add = 0,
    Subtract,
    RevSubtract,
    Min,
    Max,

    ENUM_COUNT
  };
};

struct wdGALStencilOp
{
  typedef wdUInt8 StorageType;

  enum Enum
  {
    Keep = 0,
    Zero,
    Replace,
    IncrementSaturated,
    DecrementSaturated,
    Invert,
    Increment,
    Decrement,

    ENUM_COUNT,

    Default = Keep
  };
};

struct wdGALCompareFunc
{
  typedef wdUInt8 StorageType;

  enum Enum
  {
    Never = 0,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always,

    ENUM_COUNT,

    Default = Never
  };
};

/// \brief Defines which sides of a polygon gets culled by the graphics card
struct wdGALCullMode
{
  typedef wdUInt8 StorageType;

  /// \brief Defines which sides of a polygon gets culled by the graphics card
  enum Enum
  {
    None = 0,  ///< Triangles do not get culled
    Front = 1, ///< When the 'front' of a triangle is visible, it gets culled. The rasterizer state defines which side is the 'front'. See
               ///< wdGALRasterizerStateCreationDescription for details.
    Back = 2,  ///< When the 'back'  of a triangle is visible, it gets culled. The rasterizer state defines which side is the 'front'. See
               ///< wdGALRasterizerStateCreationDescription for details.

    ENUM_COUNT,

    Default = Back
  };
};

struct wdGALTextureFilterMode
{
  typedef wdUInt8 StorageType;

  enum Enum
  {
    Point = 0,
    Linear,
    Anisotropic,

    Default = Linear
  };
};

struct wdGALUpdateMode
{
  enum Enum
  {
    Discard,
    NoOverwrite,
    CopyToTempStorage
  };
};

// Basic structs
struct wdGALTextureSubresource
{
  wdUInt32 m_uiMipLevel = 0;
  wdUInt32 m_uiArraySlice = 0;
};

struct wdGALSystemMemoryDescription
{
  void* m_pData = nullptr;
  wdUInt32 m_uiRowPitch = 0;
  wdUInt32 m_uiSlicePitch = 0;
};

/// \brief Base class for GAL objects, stores a creation description of the object and also allows for reference counting.
template <typename CreationDescription>
class wdGALObject : public wdRefCounted
{
public:
  wdGALObject(const CreationDescription& description)
    : m_Description(description)
  {
  }

  WD_ALWAYS_INLINE const CreationDescription& GetDescription() const { return m_Description; }

protected:
  const CreationDescription m_Description;
};

// Handles
namespace wdGAL
{
  typedef wdGenericId<16, 16> wd16_16Id;
  typedef wdGenericId<18, 14> wd18_14Id;
  typedef wdGenericId<20, 12> wd20_12Id;
} // namespace wdGAL

class wdGALSwapChainHandle
{
  WD_DECLARE_HANDLE_TYPE(wdGALSwapChainHandle, wdGAL::wd16_16Id);

  friend class wdGALDevice;
};

class wdGALShaderHandle
{
  WD_DECLARE_HANDLE_TYPE(wdGALShaderHandle, wdGAL::wd18_14Id);

  friend class wdGALDevice;
};

class wdGALTextureHandle
{
  WD_DECLARE_HANDLE_TYPE(wdGALTextureHandle, wdGAL::wd18_14Id);

  friend class wdGALDevice;
};

class wdGALBufferHandle
{
  WD_DECLARE_HANDLE_TYPE(wdGALBufferHandle, wdGAL::wd18_14Id);

  friend class wdGALDevice;
};

class wdGALResourceViewHandle
{
  WD_DECLARE_HANDLE_TYPE(wdGALResourceViewHandle, wdGAL::wd18_14Id);

  friend class wdGALDevice;
};

class wdGALUnorderedAccessViewHandle
{
  WD_DECLARE_HANDLE_TYPE(wdGALUnorderedAccessViewHandle, wdGAL::wd18_14Id);

  friend class wdGALDevice;
};

class wdGALRenderTargetViewHandle
{
  WD_DECLARE_HANDLE_TYPE(wdGALRenderTargetViewHandle, wdGAL::wd18_14Id);

  friend class wdGALDevice;
};

class wdGALDepthStencilStateHandle
{
  WD_DECLARE_HANDLE_TYPE(wdGALDepthStencilStateHandle, wdGAL::wd16_16Id);

  friend class wdGALDevice;
};

class wdGALBlendStateHandle
{
  WD_DECLARE_HANDLE_TYPE(wdGALBlendStateHandle, wdGAL::wd16_16Id);

  friend class wdGALDevice;
};

class wdGALRasterizerStateHandle
{
  WD_DECLARE_HANDLE_TYPE(wdGALRasterizerStateHandle, wdGAL::wd16_16Id);

  friend class wdGALDevice;
};

class wdGALSamplerStateHandle
{
  WD_DECLARE_HANDLE_TYPE(wdGALSamplerStateHandle, wdGAL::wd16_16Id);

  friend class wdGALDevice;
};

class wdGALVertexDeclarationHandle
{
  WD_DECLARE_HANDLE_TYPE(wdGALVertexDeclarationHandle, wdGAL::wd18_14Id);

  friend class wdGALDevice;
};

class wdGALQueryHandle
{
  WD_DECLARE_HANDLE_TYPE(wdGALQueryHandle, wdGAL::wd20_12Id);

  friend class wdGALDevice;
};

struct wdGALTimestampHandle
{
  WD_DECLARE_POD_TYPE();

  wdUInt64 m_uiIndex;
  wdUInt64 m_uiFrameCounter;
};

namespace wdGAL
{
  struct ModifiedRange
  {
    WD_ALWAYS_INLINE void Reset()
    {
      m_uiMin = wdInvalidIndex;
      m_uiMax = 0;
    }

    WD_FORCE_INLINE void SetToIncludeValue(wdUInt32 value)
    {
      m_uiMin = wdMath::Min(m_uiMin, value);
      m_uiMax = wdMath::Max(m_uiMax, value);
    }

    WD_FORCE_INLINE void SetToIncludeRange(wdUInt32 uiMin, wdUInt32 uiMax)
    {
      m_uiMin = wdMath::Min(m_uiMin, uiMin);
      m_uiMax = wdMath::Max(m_uiMax, uiMax);
    }

    WD_ALWAYS_INLINE bool IsValid() const { return m_uiMin <= m_uiMax; }

    WD_ALWAYS_INLINE wdUInt32 GetCount() const { return m_uiMax - m_uiMin + 1; }

    wdUInt32 m_uiMin = wdInvalidIndex;
    wdUInt32 m_uiMax = 0;
  };
} // namespace wdGAL
