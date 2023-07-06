
#pragma once

#include <Foundation/Algorithm/HashableStruct.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Types/RefCounted.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Resources/ResourceFormats.h>
#include <RendererFoundation/Shader/ShaderByteCode.h>
#include <Texture/Image/ImageEnums.h>

class wdWindowBase;

struct WD_RENDERERFOUNDATION_DLL wdShaderResourceType
{
  typedef wdUInt8 StorageType;
  enum Enum : wdUInt8
  {
    Unknown = 0,

    Texture1D = 1,
    Texture1DArray = 2,
    Texture2D = 3,
    Texture2DArray = 4,
    Texture2DMS = 5,
    Texture2DMSArray = 6,
    Texture3D = 7,
    TextureCube = 8,
    TextureCubeArray = 9,

    UAV = 10,            ///< RW textures and buffers
    ConstantBuffer = 20, ///< Constant buffers
    GenericBuffer = 21,  ///< Read only (structured) buffers
    Sampler = 22,        ///< Separate sampler states

    Default = Unknown,
  };

  static bool IsArray(wdShaderResourceType::Enum format);
};


/// \brief Defines a swap chain's present mode.
/// \sa wdGALWindowSwapChainCreationDescription
struct WD_RENDERERFOUNDATION_DLL wdGALPresentMode
{
  typedef wdUInt8 StorageType;

  enum Enum
  {
    Immediate,
    VSync,
    ENUM_COUNT,
    Default = VSync
  };
};

struct wdGALWindowSwapChainCreationDescription : public wdHashableStruct<wdGALWindowSwapChainCreationDescription>
{
  wdWindowBase* m_pWindow = nullptr;

  // Describes the format that should be used for the backbuffer.
  // Note however, that different platforms may enforce restrictions on this.
  wdGALMSAASampleCount::Enum m_SampleCount = wdGALMSAASampleCount::None;
  wdGALResourceFormat::Enum m_BackBufferFormat = wdGALResourceFormat::RGBAUByteNormalizedsRGB;
  wdEnum<wdGALPresentMode> m_InitialPresentMode = wdGALPresentMode::VSync;

  bool m_bDoubleBuffered = true;
  bool m_bAllowScreenshots = false;
};

struct wdGALSwapChainCreationDescription : public wdHashableStruct<wdGALSwapChainCreationDescription>
{
  const wdRTTI* m_pSwapChainType = nullptr;
};

struct wdGALDeviceCreationDescription
{
  bool m_bDebugDevice = false;
};

struct wdGALShaderCreationDescription : public wdHashableStruct<wdGALShaderCreationDescription>
{
  wdGALShaderCreationDescription();
  ~wdGALShaderCreationDescription();

  bool HasByteCodeForStage(wdGALShaderStage::Enum stage) const;

  wdScopedRefPointer<wdGALShaderByteCode> m_ByteCodes[wdGALShaderStage::ENUM_COUNT];
};

struct wdGALRenderTargetBlendDescription : public wdHashableStruct<wdGALRenderTargetBlendDescription>
{
  wdGALBlend::Enum m_SourceBlend = wdGALBlend::One;
  wdGALBlend::Enum m_DestBlend = wdGALBlend::One;
  wdGALBlendOp::Enum m_BlendOp = wdGALBlendOp::Add;

  wdGALBlend::Enum m_SourceBlendAlpha = wdGALBlend::One;
  wdGALBlend::Enum m_DestBlendAlpha = wdGALBlend::One;
  wdGALBlendOp::Enum m_BlendOpAlpha = wdGALBlendOp::Add;

  wdUInt8 m_uiWriteMask = 0xFF;    ///< Enables writes to color channels. Bit1 = Red Channel, Bit2 = Green Channel, Bit3 = Blue Channel, Bit4 = Alpha
                                   ///< Channel, Bit 5-8 are unused
  bool m_bBlendingEnabled = false; ///< If enabled, the color will be blended into the render target. Otherwise it will overwrite the render target.
                                   ///< Set m_uiWriteMask to 0 to disable all writes to the render target.
};

struct wdGALBlendStateCreationDescription : public wdHashableStruct<wdGALBlendStateCreationDescription>
{
  wdGALRenderTargetBlendDescription m_RenderTargetBlendDescriptions[WD_GAL_MAX_RENDERTARGET_COUNT];

  bool m_bAlphaToCoverage = false;  ///< Alpha-to-coverage can only be used with MSAA render targets. Default is false.
  bool m_bIndependentBlend = false; ///< If disabled, the blend state of the first render target is used for all render targets. Otherwise each
                                    ///< render target uses a different blend state.
};

struct wdGALStencilOpDescription : public wdHashableStruct<wdGALStencilOpDescription>
{
  wdEnum<wdGALStencilOp> m_FailOp = wdGALStencilOp::Keep;
  wdEnum<wdGALStencilOp> m_DepthFailOp = wdGALStencilOp::Keep;
  wdEnum<wdGALStencilOp> m_PassOp = wdGALStencilOp::Keep;

  wdEnum<wdGALCompareFunc> m_StencilFunc = wdGALCompareFunc::Always;
};

struct wdGALDepthStencilStateCreationDescription : public wdHashableStruct<wdGALDepthStencilStateCreationDescription>
{
  wdGALStencilOpDescription m_FrontFaceStencilOp;
  wdGALStencilOpDescription m_BackFaceStencilOp;

  wdEnum<wdGALCompareFunc> m_DepthTestFunc = wdGALCompareFunc::Less;

  bool m_bSeparateFrontAndBack = false; ///< If false, DX11 will use front face values for both front & back face values, GL will not call
                                        ///< gl*Separate() funcs
  bool m_bDepthTest = true;
  bool m_bDepthWrite = true;
  bool m_bStencilTest = false;
  wdUInt8 m_uiStencilReadMask = 0xFF;
  wdUInt8 m_uiStencilWriteMask = 0xFF;
};

/// \brief Describes the settings for a new rasterizer state. See wdGALDevice::CreateRasterizerState
struct wdGALRasterizerStateCreationDescription : public wdHashableStruct<wdGALRasterizerStateCreationDescription>
{
  wdEnum<wdGALCullMode> m_CullMode = wdGALCullMode::Back; ///< Which sides of a triangle to cull. Default is wdGALCullMode::Back
  wdInt32 m_iDepthBias = 0;                               ///< The pixel depth bias. Default is 0
  float m_fDepthBiasClamp = 0.0f;                         ///< The pixel depth bias clamp. Default is 0
  float m_fSlopeScaledDepthBias = 0.0f;                   ///< The pixel slope scaled depth bias clamp. Default is 0
  bool m_bWireFrame = false;                              ///< Whether triangles are rendered filled or as wireframe. Default is false
  bool m_bFrontCounterClockwise = false; ///< Sets which triangle winding order defines the 'front' of a triangle. If true, the front of a triangle
                                         ///< is the one where the vertices appear in counter clockwise order. Default is false
  bool m_bScissorTest = false;
  bool m_bConservativeRasterization = false; ///< Whether conservative rasterization is enabled
};

struct wdGALSamplerStateCreationDescription : public wdHashableStruct<wdGALSamplerStateCreationDescription>
{
  wdEnum<wdGALTextureFilterMode> m_MinFilter;
  wdEnum<wdGALTextureFilterMode> m_MagFilter;
  wdEnum<wdGALTextureFilterMode> m_MipFilter;

  wdEnum<wdImageAddressMode> m_AddressU;
  wdEnum<wdImageAddressMode> m_AddressV;
  wdEnum<wdImageAddressMode> m_AddressW;

  wdEnum<wdGALCompareFunc> m_SampleCompareFunc;

  wdColor m_BorderColor = wdColor::Black;

  float m_fMipLodBias = 0.0f;
  float m_fMinMip = -1.0f;
  float m_fMaxMip = 42000.0f;

  wdUInt32 m_uiMaxAnisotropy = 4;
};

struct wdGALVertexAttributeSemantic
{
  enum Enum : wdUInt8
  {
    Position,
    Normal,
    Tangent,
    Color0,
    Color1,
    Color2,
    Color3,
    Color4,
    Color5,
    Color6,
    Color7,
    TexCoord0,
    TexCoord1,
    TexCoord2,
    TexCoord3,
    TexCoord4,
    TexCoord5,
    TexCoord6,
    TexCoord7,
    TexCoord8,
    TexCoord9,

    BiTangent,
    BoneIndices0,
    BoneIndices1,
    BoneWeights0,
    BoneWeights1,

    ENUM_COUNT
  };
};

struct wdGALVertexAttribute
{
  wdGALVertexAttribute() = default;

  wdGALVertexAttribute(wdGALVertexAttributeSemantic::Enum semantic, wdGALResourceFormat::Enum format, wdUInt16 uiOffset, wdUInt8 uiVertexBufferSlot,
    bool bInstanceData);

  wdGALVertexAttributeSemantic::Enum m_eSemantic = wdGALVertexAttributeSemantic::Position;
  wdGALResourceFormat::Enum m_eFormat = wdGALResourceFormat::XYZFloat;
  wdUInt16 m_uiOffset = 0;
  wdUInt8 m_uiVertexBufferSlot = 0;
  bool m_bInstanceData = false;
};

struct WD_RENDERERFOUNDATION_DLL wdGALVertexDeclarationCreationDescription : public wdHashableStruct<wdGALVertexDeclarationCreationDescription>
{
  wdGALShaderHandle m_hShader;
  wdStaticArray<wdGALVertexAttribute, 16> m_VertexAttributes;
};

struct wdGALResourceAccess
{
  WD_ALWAYS_INLINE bool IsImmutable() const { return m_bImmutable; }

  bool m_bReadBack = false;
  bool m_bImmutable = true;
};

struct wdGALBufferType
{
  typedef wdUInt8 StorageType;

  enum Enum
  {
    Generic = 0,
    VertexBuffer,
    IndexBuffer,
    ConstantBuffer,

    ENUM_COUNT,

    Default = Generic
  };
};

struct wdGALBufferCreationDescription : public wdHashableStruct<wdGALBufferCreationDescription>
{
  wdUInt32 m_uiStructSize = 0;
  wdUInt32 m_uiTotalSize = 0;

  wdEnum<wdGALBufferType> m_BufferType = wdGALBufferType::Generic;

  bool m_bUseForIndirectArguments = false;
  bool m_bUseAsStructuredBuffer = false;
  bool m_bAllowRawViews = false;
  bool m_bStreamOutputTarget = false;
  bool m_bAllowShaderResourceView = false;
  bool m_bAllowUAV = false;

  wdGALResourceAccess m_ResourceAccess;
};

struct wdGALTextureCreationDescription : public wdHashableStruct<wdGALTextureCreationDescription>
{
  void SetAsRenderTarget(
    wdUInt32 uiWidth, wdUInt32 uiHeight, wdGALResourceFormat::Enum format, wdGALMSAASampleCount::Enum sampleCount = wdGALMSAASampleCount::None);

  wdUInt32 m_uiWidth = 0;
  wdUInt32 m_uiHeight = 0;
  wdUInt32 m_uiDepth = 1;

  wdUInt32 m_uiMipLevelCount = 1;

  wdUInt32 m_uiArraySize = 1;

  wdEnum<wdGALResourceFormat> m_Format = wdGALResourceFormat::Invalid;

  wdEnum<wdGALMSAASampleCount> m_SampleCount = wdGALMSAASampleCount::None;

  wdEnum<wdGALTextureType> m_Type = wdGALTextureType::Texture2D;

  bool m_bAllowShaderResourceView = true;
  bool m_bAllowUAV = false;
  bool m_bCreateRenderTarget = false;
  bool m_bAllowDynamicMipGeneration = false;

  wdGALResourceAccess m_ResourceAccess;

  void* m_pExisitingNativeObject = nullptr; ///< Can be used to encapsulate existing native textures in objects usable by the GAL
};

struct wdGALResourceViewCreationDescription : public wdHashableStruct<wdGALResourceViewCreationDescription>
{
  wdGALTextureHandle m_hTexture;

  wdGALBufferHandle m_hBuffer;

  wdEnum<wdGALResourceFormat> m_OverrideViewFormat = wdGALResourceFormat::Invalid;

  // Texture only
  wdUInt32 m_uiMostDetailedMipLevel = 0;
  wdUInt32 m_uiMipLevelsToUse = 0xFFFFFFFFu;

  wdUInt32 m_uiFirstArraySlice = 0; // For cubemap array: index of first 2d slice to start with
  wdUInt32 m_uiArraySize = 1;       // For cubemap array: number of cubemaps

  // Buffer only
  wdUInt32 m_uiFirstElement = 0;
  wdUInt32 m_uiNumElements = 0;
  bool m_bRawView = false;
};

struct wdGALRenderTargetViewCreationDescription : public wdHashableStruct<wdGALRenderTargetViewCreationDescription>
{
  wdGALTextureHandle m_hTexture;

  wdEnum<wdGALResourceFormat> m_OverrideViewFormat = wdGALResourceFormat::Invalid;

  wdUInt32 m_uiMipLevel = 0;

  wdUInt32 m_uiFirstSlice = 0;
  wdUInt32 m_uiSliceCount = 1;

  bool m_bReadOnly = false; ///< Can be used for depth stencil views to create read only views (e.g. for soft particles using the native depth buffer)
};

struct wdGALUnorderedAccessViewCreationDescription : public wdHashableStruct<wdGALUnorderedAccessViewCreationDescription>
{
  wdGALTextureHandle m_hTexture;

  wdGALBufferHandle m_hBuffer;

  wdEnum<wdGALResourceFormat> m_OverrideViewFormat = wdGALResourceFormat::Invalid;

  // Texture only
  wdUInt32 m_uiMipLevelToUse = 0;   ///< Which MipLevel is accessed with this UAV
  wdUInt32 m_uiFirstArraySlice = 0; ///< First depth slice for 3D Textures.
  wdUInt32 m_uiArraySize = 1;       ///< Number of depth slices for 3D textures.

  // Buffer only
  wdUInt32 m_uiFirstElement = 0;
  wdUInt32 m_uiNumElements = 0;
  bool m_bRawView = false;
  bool m_bAppend = false; // Allows appending data to the end of the buffer.
};

struct wdGALQueryType
{
  typedef wdUInt8 StorageType;

  enum Enum
  {
    /// Number of samples that passed the depth and stencil test between begin and end (on a context).
    NumSamplesPassed,
    /// Boolean version of NumSamplesPassed.
    AnySamplesPassed,

    Default = NumSamplesPassed

    // Note:
    // GALFence provides an implementation of "event queries".
  };
};

struct wdGALQueryCreationDescription : public wdHashableStruct<wdGALQueryCreationDescription>
{
  wdEnum<wdGALQueryType> m_type = wdGALQueryType::NumSamplesPassed;

  /// In case this query is used for occlusion culling (type AnySamplesPassed), this determines whether drawing should be done if the query
  /// status is still unknown.
  bool m_bDrawIfUnknown = true;
};

/// \brief Type for important GAL events.
struct wdGALDeviceEvent
{
  enum Type
  {
    AfterInit,
    BeforeShutdown,
    BeforeBeginFrame,
    AfterBeginFrame,
    BeforeEndFrame,
    AfterEndFrame,
    // could add resource creation/destruction events, if this would be useful
  };

  Type m_Type;
  class wdGALDevice* m_pDevice;
};

#include <RendererFoundation/Descriptors/Implementation/Descriptors_inl.h>
