
#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Types/RefCounted.h>
#include <RendererFoundation/RendererFoundationDLL.h>

/// \brief This class wraps shader byte code storage.
/// Since byte code can have different requirements for alignment, padding etc. this class manages it.
/// Also since byte code is shared between multiple shaders (e.g. same vertex shaders for different pixel shaders)
/// the instances of the byte codes are reference counted.
class WD_RENDERERFOUNDATION_DLL wdGALShaderByteCode : public wdRefCounted
{
public:
  wdGALShaderByteCode();

  wdGALShaderByteCode(const wdArrayPtr<const wdUInt8>& byteCode);

  inline const void* GetByteCode() const;

  inline wdUInt32 GetSize() const;

  inline bool IsValid() const;

protected:
  void CopyFrom(const wdArrayPtr<const wdUInt8>& pByteCode);

  wdDynamicArray<wdUInt8> m_Source;
};

#include <RendererFoundation/Shader/Implementation/ShaderByteCode_inl.h>
