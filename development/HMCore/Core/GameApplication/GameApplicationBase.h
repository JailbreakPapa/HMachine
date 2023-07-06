#pragma once

#include <Core/Configuration/PlatformProfile.h>
#include <Core/Console/ConsoleFunction.h>
#include <Core/GameApplication/WindowOutputTargetBase.h>
#include <Core/GameState/GameStateBase.h>
#include <Core/System/Window.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Types/UniquePtr.h>

class wdWindowBase;
struct wdWindowCreationDesc;
class wdWorld;

/// Allows custom code to inject logic at specific points during
/// initialization or during shutdown. The events are listed in
/// the order in which they typically happen.
struct wdGameApplicationStaticEvent
{
  enum class Type
  {
    AfterGameStateActivated,
    BeforeGameStateDeactivated
  };

  Type m_Type;
};

/// Allows custom code to inject logic at specific update points.
/// The events are listed in the order in which they typically happen.
struct wdGameApplicationExecutionEvent
{
  enum class Type
  {
    BeginAppTick,
    BeforeWorldUpdates,
    AfterWorldUpdates,
    BeforeUpdatePlugins,
    AfterUpdatePlugins,
    BeforePresent,
    AfterPresent,
    EndAppTick,
  };

  Type m_Type;
};

// TODO: document this and update wdGameApplication comments

class WD_CORE_DLL wdGameApplicationBase : public wdApplication
{
public:
  using SUPER = wdApplication;

  wdGameApplicationBase(const char* szAppName);
  ~wdGameApplicationBase();

  /// \name Basics
  ///@{

public:
  /// \brief Returns the wdGameApplicationBase singleton
  static wdGameApplicationBase* GetGameApplicationBaseInstance() { return s_pGameApplicationBaseInstance; }

protected:
  static wdGameApplicationBase* s_pGameApplicationBaseInstance;

  ///@}
  /// \name Capturing Data
  ///@{

public:
  /// \brief Does a profiling capture and writes it to disk at ':appdata'
  void TakeProfilingCapture();

  /// \brief Schedules a screenshot to be taken at the end of the frame.
  ///
  /// After taking a screenshot, StoreScreenshot() is executed, which may decide where to write the result to.
  void TakeScreenshot();

protected:
  /// \brief Called with the result from taking a screenshot. The default implementation writes the image to disk at ':appdata/Screenshots'
  virtual void StoreScreenshot(wdImage&& image, const char* szContext = nullptr);

  void ExecuteTakeScreenshot(wdWindowOutputTargetBase* pOutputTarget, const char* szContext = nullptr);

  bool m_bTakeScreenshot = false;

  /// expose TakeScreenshot() as a console function
  wdConsoleFunction<void()> m_ConFunc_TakeScreenshot;

  ///@}
  /// \name Frame Captures
  ///@{

public:
  /// \brief Schedules a frame capture if the corresponding plugin is loaded.
  ///
  /// If continuous capture mode is enabled the currently running frame capture is persisted (and not discarded).
  /// Otherwise, the next frame will be captured and persisted.
  void CaptureFrame();

  /// \brief Controls if frame captures are taken continuously (without being persisted) or only on-demand.
  ///
  /// If continuous frame capture is enabled, calling CaptureFrame() will persist the result of the frame capture that is
  /// currently in progress. If continuous frame capture is disabled, CaptureFrame() will capture and persist the next frame.
  /// Note that continuous capture mode comes with a performance cost, but allows the user to decide on-the-fly if the current
  /// frame capture is to be persisted, e.g., when a unit test image comparison fails.
  void SetContinuousFrameCapture(bool bEnable);
  bool GetContinousFrameCapture() const;

  /// \brief Get the absolute base output path for frame captures.
  virtual wdResult GetAbsFrameCaptureOutputPath(wdStringBuilder& ref_sOutputPath);

protected:
  void ExecuteFrameCapture(wdWindowHandle targetWindowHandle, const char* szContext = nullptr);

  bool m_bContinuousFrameCapture = false;
  bool m_bCaptureFrame = false;

  /// expose CaptureFrame() as a console function
  wdConsoleFunction<void()> m_ConFunc_CaptureFrame;

  ///@}
  /// \name GameState
  ///@{
public:
  /// \brief Creates and activates the game state for this application.
  ///
  /// If the application already has a world (such as the editor), it can pass this to the newly created game state.
  /// Otherwise the game state should create its own world.
  ///
  /// In the editor case, there are cases where a 'player start position' is specified, which can be used
  /// by the game state to place the player.
  ///
  /// Broadcasts local event: wdGameApplicationStaticEvent::AfterGameStateActivated
  /// Broadcasts global event: AfterGameStateActivation(wdGameStateBase*)
  wdResult ActivateGameState(wdWorld* pWorld = nullptr, const wdTransform* pStartPosition = nullptr);

  /// \brief Deactivates and destroys the active game state.
  ///
  /// Broadcasts local event: wdGameApplicationStaticEvent::BeforeGameStateDeactivated
  /// Broadcasts global event: BeforeGameStateDeactivation(wdGameStateBase*)
  void DeactivateGameState();

  /// \brief Returns the currently active game state. Could be nullptr.
  wdGameStateBase* GetActiveGameState() const { return m_pGameState.Borrow(); }

  /// \brief Returns the currently active game state IF it was created for the given world.
  ///
  /// This is mostly for editor use cases, where some documents want to handle the game state, but only
  /// it it was set up for a particular document.
  wdGameStateBase* GetActiveGameStateLinkedToWorld(const wdWorld* pWorld) const;

protected:
  /// \brief Creates a game state for the application to use.
  ///
  /// \a pWorld is typically nullptr in a stand-alone app, but may be existing already when called from the editor.
  ///
  /// The default implementation will query all available game states for the best match.
  /// By overriding this, one can also just create a specific game state directly.
  virtual wdUniquePtr<wdGameStateBase> CreateGameState(wdWorld* pWorld);

  /// \brief Allows to override whether a game state is created and activated at application startup.
  ///
  /// The default implementation just calls ActivateGameState(), but applications that run inside the editor override this to do nothing,
  /// as they only want the game state to become active during simulation, not during editing.
  virtual void ActivateGameStateAtStartup();

  wdUniquePtr<wdGameStateBase> m_pGameState;
  wdWorld* m_pWorldLinkedWithGameState = nullptr;

  ///@}
  /// \name Platform Profile
  ///@{
public:
  /// \brief Returns the wdPlatformProfile that has been loaded for this application
  const wdPlatformProfile& GetPlatformProfile() const { return m_PlatformProfile; }


protected:
  wdPlatformProfile m_PlatformProfile;

  ///@}
  /// \name Application Startup
  ///@{
protected:
  virtual wdResult BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;

  /// \brief Returns the target of the 'project' special data directory.
  ///
  /// The return value of this function will be passed into wdFileSystem::SetSpecialDirectory.
  /// Afterwards, any path starting with the special directory marker (">project/") will point
  /// into this directory.
  virtual wdString FindProjectDirectory() const = 0;

  /// \brief Returns the target of the 'base' data directory.
  ///
  /// Path needs to start with a special directory marker (">marker/").
  /// This is passed into the target of the 'base' data directory. Target defaults to ">sdk/Data/Base".
  virtual wdString GetBaseDataDirectoryPath() const;

  /// \brief Returns the target of the 'project' data directory.
  ///
  /// Path needs to start with a special directory marker (">marker/").
  /// This is passed into the target of the 'project' data directory. Target defaults to ">project/".
  virtual wdString GetProjectDataDirectoryPath() const;

  /// \brief Executes all 'BaseInit_' functions. Typically done very early, before core system startup
  virtual void ExecuteBaseInitFunctions();
  virtual void BaseInit_ConfigureLogging();

  wdEventSubscriptionID m_LogToConsoleID = 0;
  wdEventSubscriptionID m_LogToVsID = 0;

  /// \brief Executes all 'Init_' functions. Typically done after core system startup
  virtual void ExecuteInitFunctions();
  virtual void Init_PlatformProfile_SetPreferred();
  virtual void Init_ConfigureTelemetry();
  virtual void Init_FileSystem_SetSpecialDirs();
  virtual void Init_LoadRequiredPlugins();
  virtual void Init_ConfigureAssetManagement();
  virtual void Init_FileSystem_ConfigureDataDirs();
  virtual void Init_LoadWorldModuleConfig();
  virtual void Init_LoadProjectPlugins();
  virtual void Init_PlatformProfile_LoadForRuntime();
  virtual void Init_ConfigureInput();
  virtual void Init_ConfigureTags();
  virtual void Init_ConfigureCVars();
  virtual void Init_SetupGraphicsDevice() = 0;
  virtual void Init_SetupDefaultResources();

  wdEvent<const wdGameApplicationStaticEvent&> m_StaticEvents;

  ///@}
  /// \name Application Shutdown
  ///@{
protected:
  virtual void BeforeHighLevelSystemsShutdown() override;
  virtual void BeforeCoreSystemsShutdown() override;

  virtual void Deinit_ShutdownGraphicsDevice() = 0;
  virtual void Deinit_UnloadPlugins();
  virtual void Deinit_ShutdownLogging();

  ///@}
  /// \name Application Execution
  ///@{

public:
  virtual wdApplication::Execution Run() override;

  wdCopyOnBroadcastEvent<const wdGameApplicationExecutionEvent&> m_ExecutionEvents;

  wdTime GetFrameTime() const { return m_FrameTime; }

protected:
  virtual bool IsGameUpdateEnabled() const { return true; }

  virtual void Run_InputUpdate();
  virtual bool Run_ProcessApplicationInput();
  virtual void Run_WorldUpdateAndRender() = 0;
  virtual void Run_BeforeWorldUpdate();
  virtual void Run_AfterWorldUpdate();
  virtual void Run_UpdatePlugins();
  virtual void Run_Present();
  virtual void Run_FinishFrame();

  void UpdateFrameTime();

  wdTime m_FrameTime;
  ///@}
};
