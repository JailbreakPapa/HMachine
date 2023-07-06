#pragma once

#include <Foundation/Strings/HashedString.h>
#include <RendererCore/Pipeline/RenderData.h>

class WD_RENDERERCORE_DLL wdExtractor : public wdReflectedClass
{
  WD_ADD_DYNAMIC_REFLECTION(wdExtractor, wdReflectedClass);
  WD_DISALLOW_COPY_AND_ASSIGN(wdExtractor);

public:
  wdExtractor(const char* szName);
  virtual ~wdExtractor();

  /// \brief Sets the name of the extractor.
  void SetName(const char* szName);

  /// \brief returns the name of the extractor.
  const char* GetName() const;

  virtual void Extract(const wdView& view, const wdDynamicArray<const wdGameObject*>& visibleObjects, wdExtractedRenderData& ref_extractedRenderData);

  virtual void PostSortAndBatch(const wdView& view, const wdDynamicArray<const wdGameObject*>& visibleObjects, wdExtractedRenderData& ref_extractedRenderData);

protected:
  /// \brief returns true if the given object should be filtered by view tags.
  bool FilterByViewTags(const wdView& view, const wdGameObject* pObject) const;

  /// \brief extracts the render data for the given object.
  void ExtractRenderData(const wdView& view, const wdGameObject* pObject, wdMsgExtractRenderData& msg, wdExtractedRenderData& extractedRenderData) const;

private:
  friend class wdRenderPipeline;

  bool m_bActive;

  wdHashedString m_sName;

protected:
  wdHybridArray<wdHashedString, 4> m_DependsOn;

#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)
  mutable wdUInt32 m_uiNumCachedRenderData;
  mutable wdUInt32 m_uiNumUncachedRenderData;
#endif
};


class WD_RENDERERCORE_DLL wdVisibleObjectsExtractor : public wdExtractor
{
  WD_ADD_DYNAMIC_REFLECTION(wdVisibleObjectsExtractor, wdExtractor);

public:
  wdVisibleObjectsExtractor(const char* szName = "VisibleObjectsExtractor");
  ~wdVisibleObjectsExtractor();

  virtual void Extract(const wdView& view, const wdDynamicArray<const wdGameObject*>& visibleObjects, wdExtractedRenderData& ref_extractedRenderData) override;
};

class WD_RENDERERCORE_DLL wdSelectedObjectsExtractorBase : public wdExtractor
{
  WD_ADD_DYNAMIC_REFLECTION(wdSelectedObjectsExtractorBase, wdExtractor);

public:
  wdSelectedObjectsExtractorBase(const char* szName = "SelectedObjectsExtractor");
  ~wdSelectedObjectsExtractorBase();

  virtual void Extract(const wdView& view, const wdDynamicArray<const wdGameObject*>& visibleObjects, wdExtractedRenderData& ref_extractedRenderData) override;

  virtual const wdDeque<wdGameObjectHandle>* GetSelection() = 0;

  wdRenderData::Category m_OverrideCategory;
};

/// \brief Stores a list of game objects that should get highlighted by the renderer.
///
/// Store an instance somewhere in your game code:
/// wdSelectedObjectsContext m_SelectedObjects;
/// Add handles to game object that should be get the highlighting outline (as the editor uses for selected objects).
/// On an wdView call:
/// wdView::SetExtractorProperty("HighlightObjects", "SelectionContext", &m_SelectedObjects);
/// The first name must be the name of an wdSelectedObjectsExtractor that is instantiated by the render pipeline.
///
/// As long as there is also an wdSelectionHighlightPass in the render pipeline, all objects in this selection will be rendered
/// with an outline.
class WD_RENDERERCORE_DLL wdSelectedObjectsContext : public wdReflectedClass
{
  WD_ADD_DYNAMIC_REFLECTION(wdSelectedObjectsContext, wdReflectedClass);

public:
  wdSelectedObjectsContext();
  ~wdSelectedObjectsContext();

  void RemoveDeadObjects(const wdWorld& world);
  void AddObjectAndChildren(const wdWorld& world, const wdGameObjectHandle& hObject);
  void AddObjectAndChildren(const wdWorld& world, const wdGameObject* pObject);

  wdDeque<wdGameObjectHandle> m_Objects;
};

/// \brief An extractor that can be instantiated in a render pipeline, to define manually which objects should be rendered with a selection outline.
///
/// \sa wdSelectedObjectsContext
class WD_RENDERERCORE_DLL wdSelectedObjectsExtractor : public wdSelectedObjectsExtractorBase
{
  WD_ADD_DYNAMIC_REFLECTION(wdSelectedObjectsExtractor, wdSelectedObjectsExtractorBase);

public:
  wdSelectedObjectsExtractor(const char* szName = "ExplicitlySelectedObjectsExtractor");
  ~wdSelectedObjectsExtractor();

  virtual const wdDeque<wdGameObjectHandle>* GetSelection() override;

  /// \brief The context is typically set through an wdView, through wdView::SetExtractorProperty("<name>", "SelectionContext", pointer);
  void SetSelectionContext(wdSelectedObjectsContext* pSelectionContext) { m_pSelectionContext = pSelectionContext; } // [ property ]
  wdSelectedObjectsContext* GetSelectionContext() const { return m_pSelectionContext; }                              // [ property ]

private:
  wdSelectedObjectsContext* m_pSelectionContext = nullptr;
};
