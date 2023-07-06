#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Resources/QueryVulkan.h>

wdGALQueryVulkan::wdGALQueryVulkan(const wdGALQueryCreationDescription& Description)
  : wdGALQuery(Description)
{
}

wdGALQueryVulkan::~wdGALQueryVulkan() {}

wdResult wdGALQueryVulkan::InitPlatform(wdGALDevice* pDevice)
{
  wdGALDeviceVulkan* pVulkanDevice = static_cast<wdGALDeviceVulkan*>(pDevice);

  if (true)
  {
    return WD_SUCCESS;
  }
  else
  {
    wdLog::Error("Creation of native Vulkan query failed!");
    return WD_FAILURE;
  }
}

wdResult wdGALQueryVulkan::DeInitPlatform(wdGALDevice* pDevice)
{
  // TODO
  return WD_SUCCESS;
}

void wdGALQueryVulkan::SetDebugNamePlatform(const char* szName) const
{
  wdUInt32 uiLength = wdStringUtils::GetStringElementCount(szName);

  // TODO
}

WD_STATICLINK_FILE(RendererVulkan, RendererVulkan_Resources_Implementation_QueryVulkan);
