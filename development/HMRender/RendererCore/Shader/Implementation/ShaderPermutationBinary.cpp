#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Shader/ShaderPermutationBinary.h>

struct wdShaderPermutationBinaryVersion
{
  enum Enum : wdUInt32
  {
    Version1 = 1,
    Version2 = 2,
    Version3 = 3,
    Version4 = 4,
    Version5 = 5,

    // Increase this version number to trigger shader recompilation

    ENUM_COUNT,
    Current = ENUM_COUNT - 1
  };
};

wdShaderPermutationBinary::wdShaderPermutationBinary()
{
  for (wdUInt32 stage = 0; stage < wdGALShaderStage::ENUM_COUNT; ++stage)
    m_uiShaderStageHashes[stage] = 0;
}

wdResult wdShaderPermutationBinary::Write(wdStreamWriter& inout_stream)
{
  // write this at the beginning so that the file can be read as an wdDependencyFile
  m_DependencyFile.StoreCurrentTimeStamp();
  WD_SUCCEED_OR_RETURN(m_DependencyFile.WriteDependencyFile(inout_stream));

  const wdUInt8 uiVersion = wdShaderPermutationBinaryVersion::Current;

  if (inout_stream.WriteBytes(&uiVersion, sizeof(wdUInt8)).Failed())
    return WD_FAILURE;

  for (wdUInt32 stage = 0; stage < wdGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (inout_stream.WriteDWordValue(&m_uiShaderStageHashes[stage]).Failed())
      return WD_FAILURE;
  }

  m_StateDescriptor.Save(inout_stream);

  inout_stream << m_PermutationVars.GetCount();

  for (auto& var : m_PermutationVars)
  {
    inout_stream << var.m_sName.GetString();
    inout_stream << var.m_sValue.GetString();
  }

  return WD_SUCCESS;
}

wdResult wdShaderPermutationBinary::Read(wdStreamReader& inout_stream, bool& out_bOldVersion)
{
  WD_SUCCEED_OR_RETURN(m_DependencyFile.ReadDependencyFile(inout_stream));

  wdUInt8 uiVersion = 0;

  if (inout_stream.ReadBytes(&uiVersion, sizeof(wdUInt8)) != sizeof(wdUInt8))
    return WD_FAILURE;

  WD_ASSERT_DEV(uiVersion <= wdShaderPermutationBinaryVersion::Current, "Wrong Version {0}", uiVersion);

  out_bOldVersion = uiVersion != wdShaderPermutationBinaryVersion::Current;

  for (wdUInt32 stage = 0; stage < wdGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (inout_stream.ReadDWordValue(&m_uiShaderStageHashes[stage]).Failed())
      return WD_FAILURE;
  }

  m_StateDescriptor.Load(inout_stream);

  if (uiVersion >= wdShaderPermutationBinaryVersion::Version2)
  {
    wdUInt32 uiPermutationCount;
    inout_stream >> uiPermutationCount;

    m_PermutationVars.SetCount(uiPermutationCount);

    wdStringBuilder tmp;
    for (wdUInt32 i = 0; i < uiPermutationCount; ++i)
    {
      auto& var = m_PermutationVars[i];

      inout_stream >> tmp;
      var.m_sName.Assign(tmp.GetData());
      inout_stream >> tmp;
      var.m_sValue.Assign(tmp.GetData());
    }
  }

  return WD_SUCCESS;
}



WD_STATICLINK_FILE(RendererCore, RendererCore_Shader_Implementation_ShaderPermutationBinary);
