#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <RendererCore/Declarations.h>

//////////////////////////////////////////////////////////////////////////
// wdShaderBindFlags
//////////////////////////////////////////////////////////////////////////

struct WD_RENDERERCORE_DLL wdShaderBindFlags
{
  using StorageType = wdUInt32;

  enum Enum
  {
    None = 0,                ///< No flags causes the default shader binding behavior (all render states are applied)
    ForceRebind = WD_BIT(0), ///< Executes shader binding (and state setting), even if the shader hasn't changed. Use this, when the same shader was
                             ///< previously used with custom bound states
    NoRasterizerState =
      WD_BIT(1), ///< The rasterizer state that is associated with the shader will not be bound. Use this when you intend to bind a custom rasterizer
    NoDepthStencilState = WD_BIT(
      2), ///< The depth-stencil state that is associated with the shader will not be bound. Use this when you intend to bind a custom depth-stencil
    NoBlendState =
      WD_BIT(3), ///< The blend state that is associated with the shader will not be bound. Use this when you intend to bind a custom blend
    NoStateBinding = NoRasterizerState | NoDepthStencilState | NoBlendState,

    Default = None
  };

  struct Bits
  {
    StorageType ForceRebind : 1;
    StorageType NoRasterizerState : 1;
    StorageType NoDepthStencilState : 1;
    StorageType NoBlendState : 1;
  };
};

WD_DECLARE_FLAGS_OPERATORS(wdShaderBindFlags);

//////////////////////////////////////////////////////////////////////////
// wdRenderContextFlags
//////////////////////////////////////////////////////////////////////////

struct WD_RENDERERCORE_DLL wdRenderContextFlags
{
  using StorageType = wdUInt32;

  enum Enum
  {
    None = 0,
    ShaderStateChanged = WD_BIT(0),
    TextureBindingChanged = WD_BIT(1),
    UAVBindingChanged = WD_BIT(2),
    SamplerBindingChanged = WD_BIT(3),
    BufferBindingChanged = WD_BIT(4),
    ConstantBufferBindingChanged = WD_BIT(5),
    MeshBufferBindingChanged = WD_BIT(6),
    MaterialBindingChanged = WD_BIT(7),

    AllStatesInvalid = ShaderStateChanged | TextureBindingChanged | UAVBindingChanged | SamplerBindingChanged | BufferBindingChanged |
                       ConstantBufferBindingChanged | MeshBufferBindingChanged,
    Default = None
  };

  struct Bits
  {
    StorageType ShaderStateChanged : 1;
    StorageType TextureBindingChanged : 1;
    StorageType UAVBindingChanged : 1;
    StorageType SamplerBindingChanged : 1;
    StorageType BufferBindingChanged : 1;
    StorageType ConstantBufferBindingChanged : 1;
    StorageType MeshBufferBindingChanged : 1;
    StorageType MaterialBindingChanged : 1;
  };
};

WD_DECLARE_FLAGS_OPERATORS(wdRenderContextFlags);

//////////////////////////////////////////////////////////////////////////
// wdDefaultSamplerFlags
//////////////////////////////////////////////////////////////////////////

struct WD_RENDERERCORE_DLL wdDefaultSamplerFlags
{
  using StorageType = wdUInt32;

  enum Enum
  {
    PointFiltering = 0,
    LinearFiltering = WD_BIT(0),

    Wrap = 0,
    Clamp = WD_BIT(1)
  };

  struct Bits
  {
    StorageType LinearFiltering : 1;
    StorageType Clamp : 1;
  };
};

WD_DECLARE_FLAGS_OPERATORS(wdDefaultSamplerFlags);
