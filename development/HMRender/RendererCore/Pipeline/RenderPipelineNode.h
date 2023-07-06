#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/Declarations.h>

class wdRenderPipelineNode;

struct wdRenderPipelineNodePin
{
  WD_DECLARE_POD_TYPE();

  struct Type
  {
    using StorageType = wdUInt8;

    enum Enum
    {
      Unknown,
      Input,
      Output,
      PassThrough,

      Default = Unknown
    };
  };

  wdEnum<Type> m_Type;
  wdUInt8 m_uiInputIndex = 0xFF;
  wdUInt8 m_uiOutputIndex = 0xFF;
  wdRenderPipelineNode* m_pParent = nullptr;
};

struct wdRenderPipelineNodeInputPin : public wdRenderPipelineNodePin
{
  WD_DECLARE_POD_TYPE();

  WD_ALWAYS_INLINE wdRenderPipelineNodeInputPin() { m_Type = Type::Input; }
};

struct wdRenderPipelineNodeOutputPin : public wdRenderPipelineNodePin
{
  WD_DECLARE_POD_TYPE();

  WD_ALWAYS_INLINE wdRenderPipelineNodeOutputPin() { m_Type = Type::Output; }
};

struct wdRenderPipelineNodePassThrougPin : public wdRenderPipelineNodePin
{
  WD_DECLARE_POD_TYPE();

  WD_ALWAYS_INLINE wdRenderPipelineNodePassThrougPin() { m_Type = Type::PassThrough; }
};

class WD_RENDERERCORE_DLL wdRenderPipelineNode : public wdReflectedClass
{
  WD_ADD_DYNAMIC_REFLECTION(wdRenderPipelineNode, wdReflectedClass);

public:
  virtual ~wdRenderPipelineNode() = default;

  void InitializePins();

  wdHashedString GetPinName(const wdRenderPipelineNodePin* pPin) const;
  const wdRenderPipelineNodePin* GetPinByName(const char* szName) const;
  const wdRenderPipelineNodePin* GetPinByName(wdHashedString sName) const;
  const wdArrayPtr<const wdRenderPipelineNodePin* const> GetInputPins() const { return m_InputPins; }
  const wdArrayPtr<const wdRenderPipelineNodePin* const> GetOutputPins() const { return m_OutputPins; }

private:
  wdDynamicArray<const wdRenderPipelineNodePin*> m_InputPins;
  wdDynamicArray<const wdRenderPipelineNodePin*> m_OutputPins;
  wdHashTable<wdHashedString, const wdRenderPipelineNodePin*> m_NameToPin;
};

WD_DECLARE_REFLECTABLE_TYPE(WD_RENDERERCORE_DLL, wdRenderPipelineNodePin);
WD_DECLARE_REFLECTABLE_TYPE(WD_RENDERERCORE_DLL, wdRenderPipelineNodeInputPin);
WD_DECLARE_REFLECTABLE_TYPE(WD_RENDERERCORE_DLL, wdRenderPipelineNodeOutputPin);
WD_DECLARE_REFLECTABLE_TYPE(WD_RENDERERCORE_DLL, wdRenderPipelineNodePassThrougPin);
