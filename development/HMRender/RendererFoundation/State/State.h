
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>

class WD_RENDERERFOUNDATION_DLL wdGALBlendState : public wdGALObject<wdGALBlendStateCreationDescription>
{
public:
protected:
  wdGALBlendState(const wdGALBlendStateCreationDescription& Description);

  virtual ~wdGALBlendState();

  virtual wdResult InitPlatform(wdGALDevice* pDevice) = 0;

  virtual wdResult DeInitPlatform(wdGALDevice* pDevice) = 0;
};

class WD_RENDERERFOUNDATION_DLL wdGALDepthStencilState : public wdGALObject<wdGALDepthStencilStateCreationDescription>
{
public:
protected:
  wdGALDepthStencilState(const wdGALDepthStencilStateCreationDescription& Description);

  virtual ~wdGALDepthStencilState();

  virtual wdResult InitPlatform(wdGALDevice* pDevice) = 0;

  virtual wdResult DeInitPlatform(wdGALDevice* pDevice) = 0;
};

class WD_RENDERERFOUNDATION_DLL wdGALRasterizerState : public wdGALObject<wdGALRasterizerStateCreationDescription>
{
public:
protected:
  wdGALRasterizerState(const wdGALRasterizerStateCreationDescription& Description);

  virtual ~wdGALRasterizerState();

  virtual wdResult InitPlatform(wdGALDevice* pDevice) = 0;

  virtual wdResult DeInitPlatform(wdGALDevice* pDevice) = 0;
};

class WD_RENDERERFOUNDATION_DLL wdGALSamplerState : public wdGALObject<wdGALSamplerStateCreationDescription>
{
public:
protected:
  wdGALSamplerState(const wdGALSamplerStateCreationDescription& Description);

  virtual ~wdGALSamplerState();

  virtual wdResult InitPlatform(wdGALDevice* pDevice) = 0;

  virtual wdResult DeInitPlatform(wdGALDevice* pDevice) = 0;
};
