#pragma once

#include <Foundation/Strings/String.h>
#include <RendererCore/Declarations.h>

namespace wdShaderHelper
{
  class WD_RENDERERCORE_DLL wdTextSectionizer
  {
  public:
    void Clear();

    void AddSection(const char* szName);

    void Process(const char* szText);

    wdStringView GetSectionContent(wdUInt32 uiSection, wdUInt32& out_uiFirstLine) const;

  private:
    struct wdTextSection
    {
      wdTextSection(const char* szName)
        : m_sName(szName)

      {
      }

      void Reset()
      {
        m_szSectionStart = nullptr;
        m_Content = wdStringView();
        m_uiFirstLine = 0;
      }

      wdString m_sName;
      const char* m_szSectionStart = nullptr;
      wdStringView m_Content;
      wdUInt32 m_uiFirstLine = 0;
    };

    wdStringBuilder m_sText;
    wdHybridArray<wdTextSection, 16> m_Sections;
  };

  struct wdShaderSections
  {
    enum Enum
    {
      PLATFORMS,
      PERMUTATIONS,
      MATERIALPARAMETER,
      RENDERSTATE,
      SHADER,
      VERTEXSHADER,
      HULLSHADER,
      DOMAINSHADER,
      GEOMETRYSHADER,
      PIXELSHADER,
      COMPUTESHADER,
      TEMPLATE_VARS
    };
  };

  WD_RENDERERCORE_DLL void GetShaderSections(const char* szContent, wdTextSectionizer& out_sections);

  wdUInt32 CalculateHash(const wdArrayPtr<wdPermutationVar>& vars);
} // namespace wdShaderHelper
