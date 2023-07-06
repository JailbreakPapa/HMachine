#pragma once

#include <RendererFoundation/Resources/Query.h>

class wdGALQueryVulkan : public wdGALQuery
{
public:
  WD_ALWAYS_INLINE wdUInt32 GetID() const;
  WD_ALWAYS_INLINE vk::QueryPool GetPool() const { return nullptr; } // TODO

protected:
  friend class wdGALDeviceVulkan;
  friend class wdMemoryUtils;

  wdGALQueryVulkan(const wdGALQueryCreationDescription& Description);
  ~wdGALQueryVulkan();

  virtual wdResult InitPlatform(wdGALDevice* pDevice) override;
  virtual wdResult DeInitPlatform(wdGALDevice* pDevice) override;

  virtual void SetDebugNamePlatform(const char* szName) const override;

  wdUInt32 m_uiID;
};

#include <RendererVulkan/Resources/Implementation/QueryVulkan_inl.h>
