#include <Core/CorePCH.h>

#include <Core/ActorSystem/ActorManager.h>
#include <Core/GameApplication/GameApplicationBase.h>
#include <Core/Input/InputManager.h>
#include <Core/Interfaces/FrameCaptureInterface.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/System/Window.h>
#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Time/Timestamp.h>
#include <Texture/Image/Image.h>

wdGameApplicationBase* wdGameApplicationBase::s_pGameApplicationBaseInstance = nullptr;

wdGameApplicationBase::wdGameApplicationBase(const char* szAppName)
  : wdApplication(szAppName)
  , m_ConFunc_TakeScreenshot("TakeScreenshot", "()", wdMakeDelegate(&wdGameApplicationBase::TakeScreenshot, this))
  , m_ConFunc_CaptureFrame("CaptureFrame", "()", wdMakeDelegate(&wdGameApplicationBase::CaptureFrame, this))
{
  s_pGameApplicationBaseInstance = this;
}

wdGameApplicationBase::~wdGameApplicationBase()
{
  s_pGameApplicationBaseInstance = nullptr;
}

void AppendCurrentTimestamp(wdStringBuilder& out_sString)
{
  const wdDateTime dt = wdTimestamp::CurrentTimestamp();

  out_sString.AppendFormat("_{0}-{1}-{2}_{3}-{4}-{5}-{6}", dt.GetYear(), wdArgU(dt.GetMonth(), 2, true), wdArgU(dt.GetDay(), 2, true), wdArgU(dt.GetHour(), 2, true), wdArgU(dt.GetMinute(), 2, true), wdArgU(dt.GetSecond(), 2, true), wdArgU(dt.GetMicroseconds() / 1000, 3, true));
}

void wdGameApplicationBase::TakeProfilingCapture()
{
  class WriteProfilingDataTask final : public wdTask
  {
  public:
    wdProfilingSystem::ProfilingData m_profilingData;

    WriteProfilingDataTask() = default;
    ~WriteProfilingDataTask() = default;

  private:
    virtual void Execute() override
    {
      wdStringBuilder sPath(":appdata/Profiling/", wdApplication::GetApplicationInstance()->GetApplicationName());
      AppendCurrentTimestamp(sPath);
      sPath.Append(".json");

      wdFileWriter fileWriter;
      if (fileWriter.Open(sPath) == WD_SUCCESS)
      {
        m_profilingData.Write(fileWriter).IgnoreResult();
        wdLog::Info("Profiling capture saved to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
      }
      else
      {
        wdLog::Error("Could not write profiling capture to '{0}'.", sPath);
      }
    }
  };

  wdSharedPtr<WriteProfilingDataTask> pWriteProfilingDataTask = WD_DEFAULT_NEW(WriteProfilingDataTask);
  pWriteProfilingDataTask->ConfigureTask("Write Profiling Data", wdTaskNesting::Never);
  wdProfilingSystem::Capture(pWriteProfilingDataTask->m_profilingData);

  wdTaskSystem::StartSingleTask(pWriteProfilingDataTask, wdTaskPriority::LongRunning);
}

//////////////////////////////////////////////////////////////////////////

void wdGameApplicationBase::TakeScreenshot()
{
  m_bTakeScreenshot = true;
}

void wdGameApplicationBase::StoreScreenshot(wdImage&& image, const char* szContext /*= nullptr*/)
{
  class WriteFileTask final : public wdTask
  {
  public:
    wdImage m_Image;
    wdStringBuilder m_sPath;

    WriteFileTask() = default;
    ~WriteFileTask() = default;

  private:
    virtual void Execute() override
    {
      // get rid of Alpha channel before saving
      m_Image.Convert(wdImageFormat::R8G8B8_UNORM_SRGB).IgnoreResult();

      if (m_Image.SaveTo(m_sPath).Succeeded())
      {
        wdLog::Info("Screenshot: '{0}'", m_sPath);
      }
    }
  };

  wdSharedPtr<WriteFileTask> pWriteTask = WD_DEFAULT_NEW(WriteFileTask);
  pWriteTask->ConfigureTask("Write Screenshot", wdTaskNesting::Never);
  pWriteTask->m_Image.ResetAndMove(std::move(image));

  pWriteTask->m_sPath.Format(":appdata/Screenshots/{0}", wdApplication::GetApplicationInstance()->GetApplicationName());
  AppendCurrentTimestamp(pWriteTask->m_sPath);
  pWriteTask->m_sPath.Append(szContext);
  pWriteTask->m_sPath.Append(".png");

  // we move the file writing off to another thread to save some time
  // if we moved it to the 'FileAccess' thread, writing a screenshot would block resource loading, which can reduce game performance
  // 'LongRunning' will give it even less priority and let the task system do them in parallel to other things
  wdTaskSystem::StartSingleTask(pWriteTask, wdTaskPriority::LongRunning);
}

void wdGameApplicationBase::ExecuteTakeScreenshot(wdWindowOutputTargetBase* pOutputTarget, const char* szContext /* = nullptr*/)
{
  if (m_bTakeScreenshot)
  {
    WD_PROFILE_SCOPE("ExecuteTakeScreenshot");
    wdImage img;
    if (pOutputTarget->CaptureImage(img).Succeeded())
    {
      StoreScreenshot(std::move(img), szContext);
    }
  }
}

//////////////////////////////////////////////////////////////////////////

void wdGameApplicationBase::CaptureFrame()
{
  m_bCaptureFrame = true;
}

void wdGameApplicationBase::SetContinuousFrameCapture(bool bEnable)
{
  m_bContinuousFrameCapture = bEnable;
}

bool wdGameApplicationBase::GetContinousFrameCapture() const
{
  return m_bContinuousFrameCapture;
}


wdResult wdGameApplicationBase::GetAbsFrameCaptureOutputPath(wdStringBuilder& ref_sOutputPath)
{
  wdStringBuilder sPath = ":appdata/FrameCaptures/Capture_";
  AppendCurrentTimestamp(sPath);
  return wdFileSystem::ResolvePath(sPath, &ref_sOutputPath, nullptr);
}

void wdGameApplicationBase::ExecuteFrameCapture(wdWindowHandle targetWindowHandle, const char* szContext /*= nullptr*/)
{
  wdFrameCaptureInterface* pCaptureInterface = wdSingletonRegistry::GetSingletonInstance<wdFrameCaptureInterface>();
  if (!pCaptureInterface)
  {
    return;
  }

  WD_PROFILE_SCOPE("ExecuteFrameCapture");
  // If we still have a running capture (i.e., if no one else has taken the capture so far), finish it
  if (pCaptureInterface->IsFrameCapturing())
  {
    if (m_bCaptureFrame)
    {
      wdStringBuilder sOutputPath;
      if (GetAbsFrameCaptureOutputPath(sOutputPath).Succeeded())
      {
        sOutputPath.Append(szContext);
        pCaptureInterface->SetAbsCaptureFilePathTemplate(sOutputPath);
      }

      pCaptureInterface->EndFrameCaptureAndWriteOutput(targetWindowHandle);

      wdStringBuilder stringBuilder;
      if (pCaptureInterface->GetLastAbsCaptureFileName(stringBuilder).Succeeded())
      {
        wdLog::Info("Frame captured: '{}'", stringBuilder);
      }
      else
      {
        wdLog::Warning("Frame capture failed!");
      }
      m_bCaptureFrame = false;
    }
    else
    {
      pCaptureInterface->EndFrameCaptureAndDiscardResult(targetWindowHandle);
    }
  }

  // Start capturing the next frame if
  // (a) we want to capture the very next frame, or
  // (b) we capture every frame and later decide if we want to persist or discard it.
  if (m_bCaptureFrame || m_bContinuousFrameCapture)
  {
    pCaptureInterface->StartFrameCapture(targetWindowHandle);
  }
}

//////////////////////////////////////////////////////////////////////////

wdResult wdGameApplicationBase::ActivateGameState(wdWorld* pWorld /*= nullptr*/, const wdTransform* pStartPosition /*= nullptr*/)
{
  WD_ASSERT_DEBUG(m_pGameState == nullptr, "ActivateGameState cannot be called when another GameState is already active");

  m_pGameState = CreateGameState(pWorld);

  if (m_pGameState == nullptr)
    return WD_FAILURE;

  m_pWorldLinkedWithGameState = pWorld;
  m_pGameState->OnActivation(pWorld, pStartPosition);

  wdGameApplicationStaticEvent e;
  e.m_Type = wdGameApplicationStaticEvent::Type::AfterGameStateActivated;
  m_StaticEvents.Broadcast(e);

  WD_BROADCAST_EVENT(AfterGameStateActivation, m_pGameState.Borrow());

  return WD_SUCCESS;
}

void wdGameApplicationBase::DeactivateGameState()
{
  if (m_pGameState == nullptr)
    return;

  WD_BROADCAST_EVENT(BeforeGameStateDeactivation, m_pGameState.Borrow());

  wdGameApplicationStaticEvent e;
  e.m_Type = wdGameApplicationStaticEvent::Type::BeforeGameStateDeactivated;
  m_StaticEvents.Broadcast(e);

  m_pGameState->OnDeactivation();

  wdActorManager::GetSingleton()->DestroyAllActors(m_pGameState.Borrow());

  m_pGameState = nullptr;
}

wdGameStateBase* wdGameApplicationBase::GetActiveGameStateLinkedToWorld(const wdWorld* pWorld) const
{
  if (m_pWorldLinkedWithGameState == pWorld)
    return m_pGameState.Borrow();

  return nullptr;
}

wdUniquePtr<wdGameStateBase> wdGameApplicationBase::CreateGameState(wdWorld* pWorld)
{
  WD_LOG_BLOCK("Create Game State");

  wdUniquePtr<wdGameStateBase> pCurState;

  {
    wdInt32 iBestPriority = -1;

    for (auto pRtti = wdRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
    {
      if (!pRtti->IsDerivedFrom<wdGameStateBase>() || !pRtti->GetAllocator()->CanAllocate())
        continue;

      wdUniquePtr<wdGameStateBase> pState = pRtti->GetAllocator()->Allocate<wdGameStateBase>();

      const wdInt32 iPriority = (wdInt32)pState->DeterminePriority(pWorld);

      if (iPriority > iBestPriority)
      {
        iBestPriority = iPriority;

        pCurState = std::move(pState);
      }
    }
  }

  return pCurState;
}

void wdGameApplicationBase::ActivateGameStateAtStartup()
{
  ActivateGameState().IgnoreResult();
}

wdResult wdGameApplicationBase::BeforeCoreSystemsStartup()
{
  wdStartup::AddApplicationTag("runtime");

  ExecuteBaseInitFunctions();

  return SUPER::BeforeCoreSystemsStartup();
}

void wdGameApplicationBase::AfterCoreSystemsStartup()
{
  SUPER::AfterCoreSystemsStartup();

  ExecuteInitFunctions();

  // If one of the init functions already requested the application to quit,
  // something must have gone wrong. Don't continue initialization and let the
  // application exit.
  if (WasQuitRequested())
  {
    return;
  }

  wdStartup::StartupHighLevelSystems();

  ActivateGameStateAtStartup();
}

void wdGameApplicationBase::ExecuteBaseInitFunctions()
{
  BaseInit_ConfigureLogging();
}

void wdGameApplicationBase::BeforeHighLevelSystemsShutdown()
{
  DeactivateGameState();

  {
    // make sure that no resources continue to be streamed in, while the engine shuts down
    wdResourceManager::EngineAboutToShutdown();
    wdResourceManager::ExecuteAllResourceCleanupCallbacks();
    wdResourceManager::FreeAllUnusedResources();
  }
}

void wdGameApplicationBase::BeforeCoreSystemsShutdown()
{
  // shut down all actors and APIs that may have been in use
  if (wdActorManager::GetSingleton() != nullptr)
  {
    wdActorManager::GetSingleton()->Shutdown();
  }

  {
    wdFrameAllocator::Reset();
    wdResourceManager::FreeAllUnusedResources();
  }

  {
    Deinit_ShutdownGraphicsDevice();
    wdResourceManager::FreeAllUnusedResources();
  }

  Deinit_UnloadPlugins();

  // shut down telemetry if it was set up
  {
    wdTelemetry::CloseConnection();
  }

  Deinit_ShutdownLogging();

  SUPER::BeforeCoreSystemsShutdown();
}

static bool s_bUpdatePluginsExecuted = false;

WD_ON_GLOBAL_EVENT(GameApp_UpdatePlugins)
{
  s_bUpdatePluginsExecuted = true;
}

wdApplication::Execution wdGameApplicationBase::Run()
{
  WD_PROFILE_SCOPE("Run");
  if (m_bWasQuitRequested)
    return wdApplication::Execution::Quit;

  s_bUpdatePluginsExecuted = false;

  wdActorManager::GetSingleton()->Update();

  if (!IsGameUpdateEnabled())
    return wdApplication::Execution::Continue;

  {
    // for plugins that need to hook into this without a link dependency on this lib
    WD_PROFILE_SCOPE("GameApp_BeginAppTick");
    WD_BROADCAST_EVENT(GameApp_BeginAppTick);
    wdGameApplicationExecutionEvent e;
    e.m_Type = wdGameApplicationExecutionEvent::Type::BeginAppTick;
    m_ExecutionEvents.Broadcast(e);
  }

  Run_InputUpdate();

  Run_WorldUpdateAndRender();

  if (!s_bUpdatePluginsExecuted)
  {
    Run_UpdatePlugins();

    WD_ASSERT_DEV(s_bUpdatePluginsExecuted, "wdGameApplicationBase::Run_UpdatePlugins has been overridden, but it does not broadcast the "
                                            "global event 'GameApp_UpdatePlugins' anymore.");
  }

  {
    // for plugins that need to hook into this without a link dependency on this lib
    WD_PROFILE_SCOPE("GameApp_EndAppTick");
    WD_BROADCAST_EVENT(GameApp_EndAppTick);

    wdGameApplicationExecutionEvent e;
    e.m_Type = wdGameApplicationExecutionEvent::Type::EndAppTick;
    m_ExecutionEvents.Broadcast(e);
  }

  {
    WD_PROFILE_SCOPE("BeforePresent");
    wdGameApplicationExecutionEvent e;
    e.m_Type = wdGameApplicationExecutionEvent::Type::BeforePresent;
    m_ExecutionEvents.Broadcast(e);
  }

  {
    WD_PROFILE_SCOPE("Run_Present");
    Run_Present();
  }
  wdClock::GetGlobalClock()->Update();
  UpdateFrameTime();

  {
    WD_PROFILE_SCOPE("AfterPresent");
    wdGameApplicationExecutionEvent e;
    e.m_Type = wdGameApplicationExecutionEvent::Type::AfterPresent;
    m_ExecutionEvents.Broadcast(e);
  }

  {
    WD_PROFILE_SCOPE("Run_FinishFrame");
    Run_FinishFrame();
  }
  return wdApplication::Execution::Continue;
}

void wdGameApplicationBase::Run_InputUpdate()
{
  WD_PROFILE_SCOPE("Run_InputUpdate");
  wdInputManager::Update(wdClock::GetGlobalClock()->GetTimeDiff());

  if (!Run_ProcessApplicationInput())
    return;

  if (m_pGameState)
  {
    m_pGameState->ProcessInput();
  }
}

bool wdGameApplicationBase::Run_ProcessApplicationInput()
{
  return true;
}

void wdGameApplicationBase::Run_BeforeWorldUpdate()
{
  WD_PROFILE_SCOPE("GameApplication.BeforeWorldUpdate");

  if (m_pGameState)
  {
    m_pGameState->BeforeWorldUpdate();
  }

  {
    wdGameApplicationExecutionEvent e;
    e.m_Type = wdGameApplicationExecutionEvent::Type::BeforeWorldUpdates;
    m_ExecutionEvents.Broadcast(e);
  }
}

void wdGameApplicationBase::Run_AfterWorldUpdate()
{
  WD_PROFILE_SCOPE("GameApplication.AfterWorldUpdate");

  if (m_pGameState)
  {
    m_pGameState->AfterWorldUpdate();
  }

  {
    wdGameApplicationExecutionEvent e;
    e.m_Type = wdGameApplicationExecutionEvent::Type::AfterWorldUpdates;
    m_ExecutionEvents.Broadcast(e);
  }
}

void wdGameApplicationBase::Run_UpdatePlugins()
{
  WD_PROFILE_SCOPE("Run_UpdatePlugins");
  {
    wdGameApplicationExecutionEvent e;
    e.m_Type = wdGameApplicationExecutionEvent::Type::BeforeUpdatePlugins;
    m_ExecutionEvents.Broadcast(e);
  }

  // for plugins that need to hook into this without a link dependency on this lib
  WD_BROADCAST_EVENT(GameApp_UpdatePlugins);

  {
    wdGameApplicationExecutionEvent e;
    e.m_Type = wdGameApplicationExecutionEvent::Type::AfterUpdatePlugins;
    m_ExecutionEvents.Broadcast(e);
  }
}

void wdGameApplicationBase::Run_Present() {}

void wdGameApplicationBase::Run_FinishFrame()
{
  wdTelemetry::PerFrameUpdate();
  wdResourceManager::PerFrameUpdate();
  wdTaskSystem::FinishFrameTasks();
  wdFrameAllocator::Swap();
  wdProfilingSystem::StartNewFrame();

  // if many messages have been logged, make sure they get written to disk
  wdLog::Flush(100, wdTime::Seconds(10));

  // reset this state
  m_bTakeScreenshot = false;
}

void wdGameApplicationBase::UpdateFrameTime()
{
  // Do not use wdClock for this, it smooths and clamps the timestep
  const wdTime tNow = wdTime::Now();

  static wdTime tLast = tNow;
  m_FrameTime = tNow - tLast;
  tLast = tNow;
}

WD_STATICLINK_FILE(Core, Core_GameApplication_Implementation_GameApplicationBase);
