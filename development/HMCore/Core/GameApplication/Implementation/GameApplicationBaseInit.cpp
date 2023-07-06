#include <Core/CorePCH.h>

#include <Core/GameApplication/GameApplicationBase.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/WorldModuleConfig.h>
#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/Application/Config/PluginConfig.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/IO/Archive/DataDirTypeArchive.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Types/TagRegistry.h>
#include <Foundation/Utilities/CommandLineOptions.h>

wdCommandLineOptionBool opt_DisableConsoleOutput("app", "-disableConsoleOutput", "Disables logging to the standard console window.", false);
wdCommandLineOptionInt opt_TelemetryPort("app", "-TelemetryPort", "The network port over which telemetry is sent.", wdTelemetry::s_uiPort);
wdCommandLineOptionString opt_Profile("app", "-profile", "The platform profile to use.", "PC");

wdString wdGameApplicationBase::GetBaseDataDirectoryPath() const
{
  return ">sdk/Data/Base";
}

wdString wdGameApplicationBase::GetProjectDataDirectoryPath() const
{
  return ">project/";
}

void wdGameApplicationBase::ExecuteInitFunctions()
{
  Init_PlatformProfile_SetPreferred();
  Init_ConfigureTelemetry();
  Init_FileSystem_SetSpecialDirs();
  Init_LoadRequiredPlugins();
  Init_ConfigureAssetManagement();
  Init_FileSystem_ConfigureDataDirs();
  Init_LoadWorldModuleConfig();
  Init_LoadProjectPlugins();
  Init_PlatformProfile_LoadForRuntime();
  Init_ConfigureInput();
  Init_ConfigureTags();
  Init_ConfigureCVars();
  Init_SetupGraphicsDevice();
  Init_SetupDefaultResources();
}

void wdGameApplicationBase::Init_PlatformProfile_SetPreferred()
{
  m_PlatformProfile.m_sName = opt_Profile.GetOptionValue(wdCommandLineOption::LogMode::AlwaysIfSpecified);
  m_PlatformProfile.AddMissingConfigs();
}

void wdGameApplicationBase::BaseInit_ConfigureLogging()
{
#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)
  wdGlobalLog::RemoveLogWriter(m_LogToConsoleID);
  wdGlobalLog::RemoveLogWriter(m_LogToVsID);

  if (!opt_DisableConsoleOutput.GetOptionValue(wdCommandLineOption::LogMode::AlwaysIfSpecified))
  {
    m_LogToConsoleID = wdGlobalLog::AddLogWriter(wdLogWriter::Console::LogMessageHandler);
  }

  m_LogToVsID = wdGlobalLog::AddLogWriter(wdLogWriter::VisualStudio::LogMessageHandler);
#endif
}

void wdGameApplicationBase::Init_ConfigureTelemetry()
{
#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)
  wdTelemetry::s_uiPort = static_cast<wdUInt16>(opt_TelemetryPort.GetOptionValue(wdCommandLineOption::LogMode::AlwaysIfSpecified));
  wdTelemetry::SetServerName(GetApplicationName());
  wdTelemetry::CreateServer();
#endif
}

void wdGameApplicationBase::Init_FileSystem_SetSpecialDirs()
{
  wdFileSystem::SetSpecialDirectory("project", FindProjectDirectory());
}

void wdGameApplicationBase::Init_ConfigureAssetManagement() {}

void wdGameApplicationBase::Init_LoadRequiredPlugins()
{
  wdPlugin::InitializeStaticallyLinkedPlugins();

#if WD_ENABLED(WD_PLATFORM_WINDOWS)
  wdPlugin::LoadPlugin("XBoxControllerPlugin", wdPluginLoadFlags::PluginIsOptional).IgnoreResult();
#endif
}

void wdGameApplicationBase::Init_FileSystem_ConfigureDataDirs()
{
  // ">appdir/" and ">user/" are built-in special directories
  // see wdFileSystem::ResolveSpecialDirectory

  const wdStringBuilder sUserDataPath(">user/", GetApplicationName());

  wdFileSystem::CreateDirectoryStructure(sUserDataPath).IgnoreResult();

  wdString writableBinRoot = ">appdir/";
  wdString shaderCacheRoot = ">appdir/";

#if WD_DISABLED(WD_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
  // On platforms where this is disabled, one can usually only write to the user directory
  // e.g. on UWP and mobile platforms
  writableBinRoot = sUserDataPath;
  shaderCacheRoot = sUserDataPath;
#endif

  // for absolute paths, read-only
  wdFileSystem::AddDataDirectory("", "GameApplicationBase", ":", wdFileSystem::ReadOnly).IgnoreResult();

  // ":bin/" : writing to the binary directory
  wdFileSystem::AddDataDirectory(writableBinRoot, "GameApplicationBase", "bin", wdFileSystem::AllowWrites).IgnoreResult();

  // ":shadercache/" for reading and writing shader files
  wdFileSystem::AddDataDirectory(shaderCacheRoot, "GameApplicationBase", "shadercache", wdFileSystem::AllowWrites).IgnoreResult();

  // ":appdata/" for reading and writing app user data
  wdFileSystem::AddDataDirectory(sUserDataPath, "GameApplicationBase", "appdata", wdFileSystem::AllowWrites).IgnoreResult();

  // ":base/" for reading the core engine files
  wdFileSystem::AddDataDirectory(GetBaseDataDirectoryPath(), "GameApplicationBase", "base", wdFileSystem::DataDirUsage::ReadOnly).IgnoreResult();

  // ":project/" for reading the project specific files
  wdFileSystem::AddDataDirectory(GetProjectDataDirectoryPath(), "GameApplicationBase", "project", wdFileSystem::DataDirUsage::ReadOnly).IgnoreResult();

  // ":plugins/" for plugin specific data (optional, if it exists)
  {
    wdStringBuilder dir;
    wdFileSystem::ResolveSpecialDirectory(">sdk/Data/Plugins", dir).IgnoreResult();
    if (wdOSFile::ExistsDirectory(dir))
    {
      wdFileSystem::AddDataDirectory(">sdk/Data/Plugins", "GameApplicationBase", "plugins", wdFileSystem::DataDirUsage::ReadOnly).IgnoreResult();
    }
  }

  {
    wdApplicationFileSystemConfig appFileSystemConfig;
    appFileSystemConfig.Load();

    // get rid of duplicates that we already hard-coded above
    for (wdUInt32 i = appFileSystemConfig.m_DataDirs.GetCount(); i > 0; --i)
    {
      const wdString name = appFileSystemConfig.m_DataDirs[i - 1].m_sRootName;
      if (name.IsEqual_NoCase(":") || name.IsEqual_NoCase("bin") || name.IsEqual_NoCase("shadercache") || name.IsEqual_NoCase("appdata") || name.IsEqual_NoCase("base") || name.IsEqual_NoCase("project") || name.IsEqual_NoCase("plugins"))
      {
        appFileSystemConfig.m_DataDirs.RemoveAtAndCopy(i - 1);
      }
    }

    appFileSystemConfig.Apply();
  }
}

void wdGameApplicationBase::Init_LoadWorldModuleConfig()
{
  wdWorldModuleConfig worldModuleConfig;
  worldModuleConfig.Load();
  worldModuleConfig.Apply();
}

void wdGameApplicationBase::Init_LoadProjectPlugins()
{
  wdApplicationPluginConfig appPluginConfig;
  appPluginConfig.Load();
  appPluginConfig.Apply();
}

void wdGameApplicationBase::Init_PlatformProfile_LoadForRuntime()
{
  const wdStringBuilder sRuntimeProfileFile(":project/RuntimeConfigs/", m_PlatformProfile.m_sName, ".wdProfile");
  m_PlatformProfile.AddMissingConfigs();
  m_PlatformProfile.LoadForRuntime(sRuntimeProfileFile).IgnoreResult();
}

void wdGameApplicationBase::Init_ConfigureInput() {}

void wdGameApplicationBase::Init_ConfigureTags()
{
  WD_LOG_BLOCK("Reading Tags", "Tags.ddl");

  wdStringView sFile = ":project/RuntimeConfigs/Tags.ddl";

#if WD_ENABLED(WD_MIGRATE_RUNTIMECONFIGS)
  sFile = wdFileSystem::MigrateFileLocation(":project/Tags.ddl", sFile);
#endif

  wdFileReader file;
  if (file.Open(sFile).Failed())
  {
    wdLog::Dev("'{}' does not exist", sFile);
    return;
  }

  wdStringBuilder tmp;

  wdOpenDdlReader reader;
  if (reader.ParseDocument(file).Failed())
  {
    wdLog::Error("Failed to parse DDL data in tags file");
    return;
  }

  const wdOpenDdlReaderElement* pRoot = reader.GetRootElement();

  for (const wdOpenDdlReaderElement* pTags = pRoot->GetFirstChild(); pTags != nullptr; pTags = pTags->GetSibling())
  {
    if (!pTags->IsCustomType("Tag"))
      continue;

    const wdOpenDdlReaderElement* pName = pTags->FindChildOfType(wdOpenDdlPrimitiveType::String, "Name");

    if (!pName)
    {
      wdLog::Error("Incomplete tag declaration!");
      continue;
    }

    tmp = pName->GetPrimitivesString()[0];
    wdTagRegistry::GetGlobalRegistry().RegisterTag(tmp);
  }
}

void wdGameApplicationBase::Init_ConfigureCVars()
{
  wdCVar::SetStorageFolder(":appdata/CVars");
  wdCVar::LoadCVars();
}

void wdGameApplicationBase::Init_SetupDefaultResources()
{
  // continuously unload resources that are not in use anymore
  wdResourceManager::SetAutoFreeUnused(wdTime::Microseconds(100), wdTime::Seconds(10.0f));
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void wdGameApplicationBase::Deinit_UnloadPlugins()
{
  wdPlugin::UnloadAllPlugins();
}

void wdGameApplicationBase::Deinit_ShutdownLogging()
{
#if WD_DISABLED(WD_COMPILE_FOR_DEVELOPMENT)
  // during development, keep these loggers active
  wdGlobalLog::RemoveLogWriter(m_LogToConsoleID);
  wdGlobalLog::RemoveLogWriter(m_LogToVsID);
#endif
}



WD_STATICLINK_FILE(Core, Core_GameApplication_Implementation_GameApplicationBaseInit);
