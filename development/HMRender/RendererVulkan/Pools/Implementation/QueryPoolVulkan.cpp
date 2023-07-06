#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Pools/QueryPoolVulkan.h>

#include <RendererVulkan/Device/DeviceVulkan.h>

void wdQueryPoolVulkan::Initialize(wdGALDeviceVulkan* pDevice, wdUInt32 uiValidBits)
{
  m_pDevice = pDevice;
  m_device = pDevice->GetVulkanDevice();
  WD_ASSERT_DEV(pDevice->GetPhysicalDeviceProperties().limits.timestampComputeAndGraphics, "Timestamps not supported by hardware.");
  m_fNanoSecondsPerTick = pDevice->GetPhysicalDeviceProperties().limits.timestampPeriod;

  m_uiValidBitsMask = uiValidBits == 64 ? wdMath::MaxValue<wdUInt64>() : ((1ull << uiValidBits) - 1);
}

void wdQueryPoolVulkan::DeInitialize()
{
  for (FramePool& framePool : m_pendingFrames)
  {
    for (wdUInt32 i = 0; i < framePool.m_pools.GetCount(); i++)
    {
      m_freePools.PushBack(framePool.m_pools[i]);
    }
  }
  m_pendingFrames.Clear();
  for (TimestampPool* pPool : m_freePools)
  {
    m_device.destroyQueryPool(pPool->m_pool);
    WD_DEFAULT_DELETE(pPool);
  }
  m_freePools.Clear();
}

void wdQueryPoolVulkan::Calibrate()
{
  // To correlate CPU to GPU time, we create an event, wait a bit for the GPU to get stuck on it and then signal it.
  // We then observe the time right after on the CPU and on the GPU via a timestamp query.
  vk::EventCreateInfo eventCreateInfo;
  vk::Event event;
  VK_ASSERT_DEV(m_device.createEvent(&eventCreateInfo, nullptr, &event));

  m_device.waitIdle();
  vk::CommandBuffer cb = m_pDevice->GetCurrentCommandBuffer();
  cb.waitEvents(1, &event, vk::PipelineStageFlagBits::eHost, vk::PipelineStageFlagBits::eHost | vk::PipelineStageFlagBits::eTopOfPipe, 0, nullptr, 0, nullptr, 0, nullptr);
  auto hTimestamp = GetTimestamp();
  InsertTimestamp(cb, hTimestamp, vk::PipelineStageFlagBits::eTopOfPipe);
  vk::Fence fence = m_pDevice->Submit({}, {}, {});

  wdTime systemTS;
  wdThreadUtils::Sleep(wdTime::Milliseconds(100)); // Waiting for 100ms should be enough for the GPU to have gotten stuck on the event right?
  m_device.setEvent(event);
  systemTS = wdTime::Now();

  VK_ASSERT_DEV(m_device.waitForFences(1, &fence, true, wdMath::MaxValue<wdUInt64>()));

  wdTime gpuTS;
  GetTimestampResult(hTimestamp, gpuTS, true).AssertSuccess();
  m_gpuToCpuDelta = systemTS - gpuTS;

  m_device.destroyEvent(event);
}

void wdQueryPoolVulkan::BeginFrame(vk::CommandBuffer commandBuffer)
{
  wdUInt64 uiCurrentFrame = m_pDevice->GetCurrentFrame();
  wdUInt64 uiSafeFrame = m_pDevice->GetSafeFrame();

  m_pCurrentFrame = &m_pendingFrames.ExpandAndGetRef();
  m_pCurrentFrame->m_uiFrameCounter = uiCurrentFrame;
  m_pCurrentFrame->m_uiNextIndex = 0;
  m_pCurrentFrame->m_pools.PushBack(GetFreePool());

  // Get results
  for (FramePool& framePool : m_pendingFrames)
  {
    if (framePool.m_uiFrameCounter > uiSafeFrame)
      break;
    for (wdUInt32 i = 0; i < framePool.m_pools.GetCount(); i++)
    {
      TimestampPool* pPool = framePool.m_pools[i];

      if (!pPool->m_bReady)
      {
        wdUInt32 uiQueryCount = s_uiPoolSize;
        if (i + 1 == m_pendingFrames[0].m_pools.GetCount())
          uiQueryCount = framePool.m_uiNextIndex % s_uiPoolSize;
        if (uiQueryCount > 0)
        {
          vk::Result res = m_device.getQueryPoolResults(pPool->m_pool, 0, uiQueryCount, s_uiPoolSize * sizeof(wdUInt64), pPool->m_queryResults.GetData(), sizeof(wdUInt64), vk::QueryResultFlagBits::e64);
          if (res == vk::Result::eSuccess)
          {
            pPool->m_bReady = true;
          }
        }
      }
    }
  }

  // Clear out old frames
  while (m_pendingFrames[0].m_uiFrameCounter + s_uiRetainFrames < uiSafeFrame)
  {
    for (TimestampPool* pPool : m_pendingFrames[0].m_pools)
    {
      pPool->m_bReady = false;
      m_freePools.PushBack(pPool);
      m_resetPools.PushBack(pPool->m_pool);
    }
    m_pendingFrames.PopFront();
  }

  m_uiFirstFrameIndex = m_pendingFrames[0].m_uiFrameCounter;

  if (m_gpuToCpuDelta.IsZero())
  {
    Calibrate();
  }

  for (vk::QueryPool pool : m_resetPools)
  {
    commandBuffer.resetQueryPool(pool, 0, s_uiPoolSize);
  }
  m_resetPools.Clear();
}

wdGALTimestampHandle wdQueryPoolVulkan::GetTimestamp()
{
  const wdUInt64 uiPoolIndex = m_pCurrentFrame->m_uiNextIndex / s_uiPoolSize;
  if (uiPoolIndex == m_pCurrentFrame->m_pools.GetCount())
  {
    m_pCurrentFrame->m_pools.PushBack(GetFreePool());
  }

  wdGALTimestampHandle res = {m_pCurrentFrame->m_uiNextIndex, m_pCurrentFrame->m_uiFrameCounter};
  m_pCurrentFrame->m_uiNextIndex++;
  return res;
}

void wdQueryPoolVulkan::InsertTimestamp(vk::CommandBuffer commandBuffer, wdGALTimestampHandle hTimestamp, vk::PipelineStageFlagBits pipelineStage)
{
  for (vk::QueryPool pool : m_resetPools)
  {
    commandBuffer.resetQueryPool(pool, 0, s_uiPoolSize);
  }
  m_resetPools.Clear();
  const wdUInt32 uiPoolIndex = (wdUInt32)hTimestamp.m_uiIndex / s_uiPoolSize;
  const wdUInt32 uiQueryIndex = (wdUInt32)hTimestamp.m_uiIndex % s_uiPoolSize;
  WD_ASSERT_DEBUG(hTimestamp.m_uiFrameCounter == m_pCurrentFrame->m_uiFrameCounter, "Timestamps must be created and used in the same frame!");

  commandBuffer.writeTimestamp(pipelineStage, m_pCurrentFrame->m_pools[uiPoolIndex]->m_pool, uiQueryIndex);
}

wdResult wdQueryPoolVulkan::GetTimestampResult(wdGALTimestampHandle hTimestamp, wdTime& result, bool bForce)
{
  if (hTimestamp.m_uiFrameCounter >= m_uiFirstFrameIndex)
  {
    const wdUInt32 uiFrameIndex = static_cast<wdUInt32>(hTimestamp.m_uiFrameCounter - m_uiFirstFrameIndex);
    const wdUInt32 uiPoolIndex = (wdUInt32)hTimestamp.m_uiIndex / s_uiPoolSize;
    const wdUInt32 uiQueryIndex = (wdUInt32)hTimestamp.m_uiIndex % s_uiPoolSize;
    FramePool& framePools = m_pendingFrames[uiFrameIndex];
    TimestampPool* pPool = framePools.m_pools[uiPoolIndex];
    if (pPool->m_bReady)
    {
      result = wdTime::Nanoseconds(m_fNanoSecondsPerTick * pPool->m_queryResults[uiQueryIndex]) + m_gpuToCpuDelta;
      return WD_SUCCESS;
    }
    else if (bForce)
    {
      vk::Result res = m_device.getQueryPoolResults(pPool->m_pool, uiQueryIndex, 1, sizeof(wdUInt64), &pPool->m_queryResults[uiQueryIndex], sizeof(wdUInt64), vk::QueryResultFlagBits::e64);
      result = wdTime::Nanoseconds(m_fNanoSecondsPerTick * pPool->m_queryResults[uiQueryIndex]) + m_gpuToCpuDelta;
      return res == vk::Result::eSuccess ? WD_SUCCESS : WD_FAILURE;
    }
    return WD_FAILURE;
  }
  else
  {
    // expired
    result = wdTime();
    return WD_SUCCESS;
  }
}

wdQueryPoolVulkan::TimestampPool* wdQueryPoolVulkan::GetFreePool()
{
  if (!m_freePools.IsEmpty())
  {
    TimestampPool* pPool = m_freePools.PeekBack();
    m_freePools.PopBack();
    pPool->m_queryResults.Clear();
    pPool->m_queryResults.SetCount(s_uiPoolSize, 0);
    return pPool;
  }

  vk::QueryPoolCreateInfo info;
  info.queryType = vk::QueryType::eTimestamp;
  info.queryCount = s_uiPoolSize;

  TimestampPool* pPool = WD_DEFAULT_NEW(TimestampPool);
  pPool->m_queryResults.SetCount(s_uiPoolSize, 0);
  VK_ASSERT_DEV(m_device.createQueryPool(&info, nullptr, &pPool->m_pool));

  m_resetPools.PushBack(pPool->m_pool);
  return pPool;
}
