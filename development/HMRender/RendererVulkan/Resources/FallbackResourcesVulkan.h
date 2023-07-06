#pragma once
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

#include <vulkan/vulkan.hpp>

class wdGALDeviceVulkan;
class wdGALResourceViewVulkan;
class wdGALUnorderedAccessViewVulkan;

/// \brief Creates fallback resources in case the high-level renderer did not map a resource to a descriptor slot.
class wdFallbackResourcesVulkan
{
public:
  static void Initialize(wdGALDeviceVulkan* pDevice);
  static void DeInitialize();

  static const wdGALResourceViewVulkan* GetFallbackResourceView(vk::DescriptorType descriptorType, wdShaderResourceType::Enum wdType, bool bDepth);
  static const wdGALUnorderedAccessViewVulkan* GetFallbackUnorderedAccessView(vk::DescriptorType descriptorType, wdShaderResourceType::Enum wdType);

private:
  static void GALDeviceEventHandler(const wdGALDeviceEvent& e);

  static wdGALDeviceVulkan* s_pDevice;
  static wdEventSubscriptionID s_EventID;

  struct Key
  {
    WD_DECLARE_POD_TYPE();
    vk::DescriptorType m_descriptorType;
    wdShaderResourceType::Enum m_wdType;
    bool m_bDepth = false;
  };

  struct KeyHash
  {
    static wdUInt32 Hash(const Key& a);
    static bool Equal(const Key& a, const Key& b);
  };

  static wdHashTable<Key, wdGALResourceViewHandle, KeyHash> m_ResourceViews;
  static wdHashTable<Key, wdGALUnorderedAccessViewHandle, KeyHash> m_UAVs;

  static wdDynamicArray<wdGALBufferHandle> m_Buffers;
  static wdDynamicArray<wdGALTextureHandle> m_Textures;
};
