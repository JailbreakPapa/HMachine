#include <RendererFoundation/RendererFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Profiling/Profiling.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Profiling/Profiling.h>

#if WD_ENABLED(WD_USE_PROFILING)

struct GPUTimingScope
{
  WD_DECLARE_POD_TYPE();

  wdGALTimestampHandle m_BeginTimestamp;
  wdGALTimestampHandle m_EndTimestamp;
  char m_szName[48];
};

class GPUProfilingSystem
{
public:
  static void ProcessTimestamps(const wdGALDeviceEvent& e)
  {
    if (e.m_Type != wdGALDeviceEvent::AfterEndFrame)
      return;

    while (!s_TimingScopes.IsEmpty())
    {
      auto& timingScope = s_TimingScopes.PeekFront();

      wdTime endTime;
      if (e.m_pDevice->GetTimestampResult(timingScope.m_EndTimestamp, endTime).Succeeded())
      {
        wdTime beginTime;
        WD_VERIFY(e.m_pDevice->GetTimestampResult(timingScope.m_BeginTimestamp, beginTime).Succeeded(),
          "Begin timestamp should be finished before end timestamp");

        if (!beginTime.IsZero() && !endTime.IsZero())
        {
#  if WD_ENABLED(WD_COMPILE_FOR_DEBUG)
          static bool warnOnRingBufferOverun = true;
          if (warnOnRingBufferOverun && endTime < beginTime)
          {
            warnOnRingBufferOverun = false;
            wdLog::Error("Profiling end is before start, the DX11 timestamp ring buffer was probably overrun.");
          }
#  endif
          wdProfilingSystem::AddGPUScope(timingScope.m_szName, beginTime, endTime);
        }

        s_TimingScopes.PopFront();
      }
      else
      {
        // Timestamps are not available yet
        break;
      }
    }
  }

  static GPUTimingScope& AllocateScope() { return s_TimingScopes.ExpandAndGetRef(); }

private:
  static void OnEngineStartup() { wdGALDevice::GetDefaultDevice()->m_Events.AddEventHandler(&GPUProfilingSystem::ProcessTimestamps); }

  static void OnEngineShutdown()
  {
    s_TimingScopes.Clear();
    wdGALDevice::GetDefaultDevice()->m_Events.RemoveEventHandler(&GPUProfilingSystem::ProcessTimestamps);
  }

  static wdDeque<GPUTimingScope, wdStaticAllocatorWrapper> s_TimingScopes;

  WD_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererFoundation, GPUProfilingSystem);
};

// clang-format off
WD_BEGIN_SUBSYSTEM_DECLARATION(RendererFoundation, GPUProfilingSystem)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    GPUProfilingSystem::OnEngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    GPUProfilingSystem::OnEngineShutdown();
  }

WD_END_SUBSYSTEM_DECLARATION;
// clang-format on

wdDeque<GPUTimingScope, wdStaticAllocatorWrapper> GPUProfilingSystem::s_TimingScopes;

//////////////////////////////////////////////////////////////////////////

GPUTimingScope* wdProfilingScopeAndMarker::Start(wdGALCommandEncoder* pCommandEncoder, const char* szName)
{
  pCommandEncoder->PushMarker(szName);

  auto& timingScope = GPUProfilingSystem::AllocateScope();
  timingScope.m_BeginTimestamp = pCommandEncoder->InsertTimestamp();
  wdStringUtils::Copy(timingScope.m_szName, WD_ARRAY_SIZE(timingScope.m_szName), szName);

  return &timingScope;
}

void wdProfilingScopeAndMarker::Stop(wdGALCommandEncoder* pCommandEncoder, GPUTimingScope*& ref_pTimingScope)
{
  pCommandEncoder->PopMarker();
  ref_pTimingScope->m_EndTimestamp = pCommandEncoder->InsertTimestamp();
  ref_pTimingScope = nullptr;
}

wdProfilingScopeAndMarker::wdProfilingScopeAndMarker(wdGALCommandEncoder* pCommandEncoder, const char* szName)
  : wdProfilingScope(szName, nullptr, wdTime::Zero())
  , m_pCommandEncoder(pCommandEncoder)
{
  m_pTimingScope = Start(pCommandEncoder, szName);
}

wdProfilingScopeAndMarker::~wdProfilingScopeAndMarker()
{
  Stop(m_pCommandEncoder, m_pTimingScope);
}

#endif

WD_STATICLINK_FILE(RendererFoundation, RendererFoundation_Profiling_Implementation_Profiling);
