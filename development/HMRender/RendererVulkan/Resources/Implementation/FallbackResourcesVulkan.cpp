#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Resources/FallbackResourcesVulkan.h>

#include <Foundation/Algorithm/HashStream.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Resources/ResourceViewVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Resources/UnorderedAccessViewVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

wdGALDeviceVulkan* wdFallbackResourcesVulkan::s_pDevice = nullptr;
wdEventSubscriptionID wdFallbackResourcesVulkan::s_EventID = 0;

wdHashTable<wdFallbackResourcesVulkan::Key, wdGALResourceViewHandle, wdFallbackResourcesVulkan::KeyHash> wdFallbackResourcesVulkan::m_ResourceViews;
wdHashTable<wdFallbackResourcesVulkan::Key, wdGALUnorderedAccessViewHandle, wdFallbackResourcesVulkan::KeyHash> wdFallbackResourcesVulkan::m_UAVs;
wdDynamicArray<wdGALBufferHandle> wdFallbackResourcesVulkan::m_Buffers;
wdDynamicArray<wdGALTextureHandle> wdFallbackResourcesVulkan::m_Textures;

void wdFallbackResourcesVulkan::Initialize(wdGALDeviceVulkan* pDevice)
{
  s_pDevice = pDevice;
  s_EventID = pDevice->m_Events.AddEventHandler(wdMakeDelegate(&wdFallbackResourcesVulkan::GALDeviceEventHandler));
}

void wdFallbackResourcesVulkan::DeInitialize()
{
  s_pDevice->m_Events.RemoveEventHandler(s_EventID);
  s_pDevice = nullptr;
}
void wdFallbackResourcesVulkan::GALDeviceEventHandler(const wdGALDeviceEvent& e)
{
  switch (e.m_Type)
  {
    case wdGALDeviceEvent::AfterInit:
    {
      auto CreateTexture = [](wdGALTextureType::Enum type, wdGALMSAASampleCount::Enum samples, bool bDepth) -> wdGALResourceViewHandle {
        wdGALTextureCreationDescription desc;
        desc.m_uiWidth = 4;
        desc.m_uiHeight = 4;
        if (type == wdGALTextureType::Texture3D)
          desc.m_uiDepth = 4;
        desc.m_uiMipLevelCount = 1;
        desc.m_Format = bDepth ? wdGALResourceFormat::D16 : wdGALResourceFormat::BGRAUByteNormalizedsRGB;
        desc.m_Type = type;
        desc.m_SampleCount = samples;
        desc.m_ResourceAccess.m_bImmutable = false;
        desc.m_bCreateRenderTarget = bDepth;
        wdGALTextureHandle hTexture = s_pDevice->CreateTexture(desc);
        WD_ASSERT_DEV(!hTexture.IsInvalidated(), "Failed to create fallback resource");
        // Debug device not set yet.
        s_pDevice->GetTexture(hTexture)->SetDebugName("FallbackResourceVulkan");
        m_Textures.PushBack(hTexture);
        return s_pDevice->GetDefaultResourceView(hTexture);
      };
      {
        wdGALResourceViewHandle hView = CreateTexture(wdGALTextureType::Texture2D, wdGALMSAASampleCount::None, false);
        m_ResourceViews[{vk::DescriptorType::eSampledImage, wdShaderResourceType::Texture2D, false}] = hView;
        m_ResourceViews[{vk::DescriptorType::eSampledImage, wdShaderResourceType::Texture2DArray, false}] = hView;
      }
      {
        wdGALResourceViewHandle hView = CreateTexture(wdGALTextureType::Texture2D, wdGALMSAASampleCount::None, true);
        m_ResourceViews[{vk::DescriptorType::eSampledImage, wdShaderResourceType::Texture2D, true}] = hView;
        m_ResourceViews[{vk::DescriptorType::eSampledImage, wdShaderResourceType::Texture2DArray, true}] = hView;
      }

      // Swift shader can only do 4x MSAA. Add a check anyways.
      vk::ImageFormatProperties props;
      vk::Result res = s_pDevice->GetVulkanPhysicalDevice().getImageFormatProperties(vk::Format::eB8G8R8A8Srgb, vk::ImageType::e2D, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eSampled, {}, &props);
      if (res == vk::Result::eSuccess && props.sampleCounts & vk::SampleCountFlagBits::e4)
      {
        wdGALResourceViewHandle hView = CreateTexture(wdGALTextureType::Texture2D, wdGALMSAASampleCount::FourSamples, false);
        m_ResourceViews[{vk::DescriptorType::eSampledImage, wdShaderResourceType::Texture2DMS, false}] = hView;
        m_ResourceViews[{vk::DescriptorType::eSampledImage, wdShaderResourceType::Texture2DMSArray, false}] = hView;
      }
      {
        wdGALResourceViewHandle hView = CreateTexture(wdGALTextureType::TextureCube, wdGALMSAASampleCount::None, false);
        m_ResourceViews[{vk::DescriptorType::eSampledImage, wdShaderResourceType::TextureCube, false}] = hView;
        m_ResourceViews[{vk::DescriptorType::eSampledImage, wdShaderResourceType::TextureCubeArray, false}] = hView;
      }
      {
        wdGALResourceViewHandle hView = CreateTexture(wdGALTextureType::Texture3D, wdGALMSAASampleCount::None, false);
        m_ResourceViews[{vk::DescriptorType::eSampledImage, wdShaderResourceType::Texture3D, false}] = hView;
      }
      {
        wdGALBufferCreationDescription desc;
        desc.m_bUseForIndirectArguments = false;
        desc.m_bUseAsStructuredBuffer = true;
        desc.m_bAllowRawViews = true;
        desc.m_bStreamOutputTarget = false;
        desc.m_bAllowShaderResourceView = true;
        desc.m_bAllowUAV = true;
        desc.m_uiStructSize = 128;
        desc.m_uiTotalSize = 1280;
        desc.m_ResourceAccess.m_bImmutable = false;
        wdGALBufferHandle hBuffer = s_pDevice->CreateBuffer(desc);
        s_pDevice->GetBuffer(hBuffer)->SetDebugName("FallbackStructuredBufferVulkan");
        m_Buffers.PushBack(hBuffer);
        wdGALResourceViewHandle hView = s_pDevice->GetDefaultResourceView(hBuffer);
        m_ResourceViews[{vk::DescriptorType::eUniformBuffer, wdShaderResourceType::ConstantBuffer, false}] = hView;
        m_ResourceViews[{vk::DescriptorType::eUniformBuffer, wdShaderResourceType::ConstantBuffer, true}] = hView;
        m_ResourceViews[{vk::DescriptorType::eStorageBuffer, wdShaderResourceType::GenericBuffer, false}] = hView;
        m_ResourceViews[{vk::DescriptorType::eStorageBuffer, wdShaderResourceType::GenericBuffer, true}] = hView;
      }
      {
        wdGALBufferCreationDescription desc;
        desc.m_uiStructSize = sizeof(wdUInt32);
        desc.m_uiTotalSize = 1024;
        desc.m_bAllowShaderResourceView = true;
        desc.m_ResourceAccess.m_bImmutable = false;
        wdGALBufferHandle hBuffer = s_pDevice->CreateBuffer(desc);
        s_pDevice->GetBuffer(hBuffer)->SetDebugName("FallbackTexelBufferVulkan");
        m_Buffers.PushBack(hBuffer);
        wdGALResourceViewHandle hView = s_pDevice->GetDefaultResourceView(hBuffer);
        m_ResourceViews[{vk::DescriptorType::eUniformTexelBuffer, wdShaderResourceType::GenericBuffer, false}] = hView;
        m_ResourceViews[{vk::DescriptorType::eUniformTexelBuffer, wdShaderResourceType::GenericBuffer, true}] = hView;
      }
    }
    break;
    case wdGALDeviceEvent::BeforeShutdown:
    {
      m_ResourceViews.Clear();
      m_ResourceViews.Compact();

      m_UAVs.Clear();
      m_UAVs.Compact();

      for (wdGALBufferHandle hBuffer : m_Buffers)
      {
        s_pDevice->DestroyBuffer(hBuffer);
      }
      m_Buffers.Clear();
      m_Buffers.Compact();

      for (wdGALTextureHandle hTexture : m_Textures)
      {
        s_pDevice->DestroyTexture(hTexture);
      }
      m_Textures.Clear();
      m_Textures.Compact();
    }
    break;
    default:
      break;
  }
}

const wdGALResourceViewVulkan* wdFallbackResourcesVulkan::GetFallbackResourceView(vk::DescriptorType descriptorType, wdShaderResourceType::Enum wdType, bool bDepth)
{
  if (wdGALResourceViewHandle* pView = m_ResourceViews.GetValue(Key{descriptorType, wdType, bDepth}))
  {
    return static_cast<const wdGALResourceViewVulkan*>(s_pDevice->GetResourceView(*pView));
  }
  WD_REPORT_FAILURE("No fallback resource set, update wdFallbackResourcesVulkan::GALDeviceEventHandler.");
  return nullptr;
}

const wdGALUnorderedAccessViewVulkan* wdFallbackResourcesVulkan::GetFallbackUnorderedAccessView(vk::DescriptorType descriptorType, wdShaderResourceType::Enum wdType)
{
  if (wdGALUnorderedAccessViewHandle* pView = m_UAVs.GetValue(Key{descriptorType, wdType, false}))
  {
    return static_cast<const wdGALUnorderedAccessViewVulkan*>(s_pDevice->GetUnorderedAccessView(*pView));
  }
  WD_REPORT_FAILURE("No fallback resource set, update wdFallbackResourcesVulkan::GALDeviceEventHandler.");
  return nullptr;
}

wdUInt32 wdFallbackResourcesVulkan::KeyHash::Hash(const Key& a)
{
  wdHashStreamWriter32 writer;
  writer << wdConversionUtilsVulkan::GetUnderlyingValue(a.m_descriptorType);
  writer << wdConversionUtilsVulkan::GetUnderlyingValue(a.m_wdType);
  writer << a.m_bDepth;
  return writer.GetHashValue();
}

bool wdFallbackResourcesVulkan::KeyHash::Equal(const Key& a, const Key& b)
{
  return a.m_descriptorType == b.m_descriptorType && a.m_wdType == b.m_wdType && a.m_bDepth == b.m_bDepth;
}
