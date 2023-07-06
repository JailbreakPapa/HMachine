#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Shader/VertexDeclarationDX11.h>
#include <RendererFoundation/Shader/Shader.h>

#include <d3d11.h>

wdGALVertexDeclarationDX11::wdGALVertexDeclarationDX11(const wdGALVertexDeclarationCreationDescription& Description)
  : wdGALVertexDeclaration(Description)
  , m_pDXInputLayout(nullptr)
{
}

wdGALVertexDeclarationDX11::~wdGALVertexDeclarationDX11() = default;

static const char* GALSemanticToDX11[] = {"POSITION", "NORMAL", "TANGENT", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR",
  "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "BITANGENT", "BONEINDICES",
  "BONEINDICES", "BONEWEIGHTS", "BONEWEIGHTS"};

static UINT GALSemanticToIndexDX11[] = {0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 1, 0, 1};

WD_CHECK_AT_COMPILETIME_MSG(WD_ARRAY_SIZE(GALSemanticToDX11) == wdGALVertexAttributeSemantic::ENUM_COUNT,
  "GALSemanticToDX11 array size does not match vertex attribute semantic count");
WD_CHECK_AT_COMPILETIME_MSG(WD_ARRAY_SIZE(GALSemanticToIndexDX11) == wdGALVertexAttributeSemantic::ENUM_COUNT,
  "GALSemanticToIndexDX11 array size does not match vertex attribute semantic count");

WD_DEFINE_AS_POD_TYPE(D3D11_INPUT_ELEMENT_DESC);

wdResult wdGALVertexDeclarationDX11::InitPlatform(wdGALDevice* pDevice)
{
  wdHybridArray<D3D11_INPUT_ELEMENT_DESC, 8> DXInputElementDescs;

  wdGALDeviceDX11* pDXDevice = static_cast<wdGALDeviceDX11*>(pDevice);

  const wdGALShader* pShader = pDevice->GetShader(m_Description.m_hShader);

  if (pShader == nullptr || !pShader->GetDescription().HasByteCodeForStage(wdGALShaderStage::VertexShader))
  {
    return WD_FAILURE;
  }

  // Copy attribute descriptions
  for (wdUInt32 i = 0; i < m_Description.m_VertexAttributes.GetCount(); i++)
  {
    const wdGALVertexAttribute& Current = m_Description.m_VertexAttributes[i];

    D3D11_INPUT_ELEMENT_DESC DXDesc;
    DXDesc.AlignedByteOffset = Current.m_uiOffset;
    DXDesc.Format = pDXDevice->GetFormatLookupTable().GetFormatInfo(Current.m_eFormat).m_eVertexAttributeType;

    if (DXDesc.Format == DXGI_FORMAT_UNKNOWN)
    {
      wdLog::Error("Vertex attribute format {0} of attribute at index {1} is unknown!", Current.m_eFormat, i);
      return WD_FAILURE;
    }

    DXDesc.InputSlot = Current.m_uiVertexBufferSlot;
    DXDesc.InputSlotClass = Current.m_bInstanceData ? D3D11_INPUT_PER_INSTANCE_DATA : D3D11_INPUT_PER_VERTEX_DATA;
    DXDesc.InstanceDataStepRate = Current.m_bInstanceData ? 1 : 0; /// \todo Expose step rate?
    DXDesc.SemanticIndex = GALSemanticToIndexDX11[Current.m_eSemantic];
    DXDesc.SemanticName = GALSemanticToDX11[Current.m_eSemantic];

    DXInputElementDescs.PushBack(DXDesc);
  }


  const wdScopedRefPointer<wdGALShaderByteCode>& pByteCode = pShader->GetDescription().m_ByteCodes[wdGALShaderStage::VertexShader];

  if (FAILED(pDXDevice->GetDXDevice()->CreateInputLayout(
        &DXInputElementDescs[0], DXInputElementDescs.GetCount(), pByteCode->GetByteCode(), pByteCode->GetSize(), &m_pDXInputLayout)))
  {
    return WD_FAILURE;
  }
  else
  {
    return WD_SUCCESS;
  }
}

wdResult wdGALVertexDeclarationDX11::DeInitPlatform(wdGALDevice* pDevice)
{
  WD_GAL_DX11_RELEASE(m_pDXInputLayout);
  return WD_SUCCESS;
}



WD_STATICLINK_FILE(RendererDX11, RendererDX11_Shader_Implementation_VertexDeclarationDX11);
