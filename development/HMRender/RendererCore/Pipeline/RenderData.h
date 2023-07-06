#pragma once

#include <Foundation/Communication/Message.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/Pipeline/Declarations.h>

class wdRasterizerObject;

/// \brief Base class for all render data. Render data must contain all information that is needed to render the corresponding object.
class WD_RENDERERCORE_DLL wdRenderData : public wdReflectedClass
{
  WD_ADD_DYNAMIC_REFLECTION(wdRenderData, wdReflectedClass);

public:
  struct Category
  {
    Category();
    explicit Category(wdUInt16 uiValue);

    bool operator==(const Category& other) const;
    bool operator!=(const Category& other) const;

    wdUInt16 m_uiValue;
  };

  struct Caching
  {
    enum Enum
    {
      Never,
      IfStatic
    };
  };

  /// \brief This function generates a 64bit sorting key for the given render data. Data with lower sorting key is rendered first.
  using SortingKeyFunc = wdDelegate<wdUInt64(const wdRenderData*, wdUInt32, const wdCamera&)>;

  static Category RegisterCategory(const char* szCategoryName, SortingKeyFunc sortingKeyFunc);
  static Category FindCategory(const char* szCategoryName);

  static const wdRenderer* GetCategoryRenderer(Category category, const wdRTTI* pRenderDataType);

  static const char* GetCategoryName(Category category);

  wdUInt64 GetCategorySortingKey(Category category, const wdCamera& camera) const;

  wdUInt32 m_uiBatchId = 0; ///< BatchId is used to group render data in batches.
  wdUInt32 m_uiSortingKey = 0;

  wdTransform m_GlobalTransform = wdTransform::IdentityTransform();
  wdBoundingBoxSphere m_GlobalBounds;

  wdGameObjectHandle m_hOwner;

#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)
  const wdGameObject* m_pOwner = nullptr; ///< Debugging only. It is not allowed to access the game object during rendering.
#endif

private:
  WD_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererCore, RenderData);

  static void PluginEventHandler(const wdPluginEvent& e);
  static void UpdateRendererTypes();

  static void CreateRendererInstances();
  static void ClearRendererInstances();

  struct CategoryData
  {
    wdHashedString m_sName;
    SortingKeyFunc m_sortingKeyFunc;

    wdHashTable<const wdRTTI*, wdUInt32> m_TypeToRendererIndex;
  };

  static wdHybridArray<CategoryData, 32> s_CategoryData;

  static wdHybridArray<const wdRTTI*, 16> s_RendererTypes;
  static wdDynamicArray<wdUniquePtr<wdRenderer>> s_RendererInstances;
  static bool s_bRendererInstancesDirty;
};

/// \brief Creates render data that is only valid for this frame. The data is automatically deleted after the frame has been rendered.
template <typename T>
static T* wdCreateRenderDataForThisFrame(const wdGameObject* pOwner);

struct WD_RENDERERCORE_DLL wdDefaultRenderDataCategories
{
  static wdRenderData::Category Light;
  static wdRenderData::Category Decal;
  static wdRenderData::Category ReflectionProbe;
  static wdRenderData::Category Sky;
  static wdRenderData::Category LitOpaque;
  static wdRenderData::Category LitMasked;
  static wdRenderData::Category LitTransparent;
  static wdRenderData::Category LitForeground;
  static wdRenderData::Category SimpleOpaque;
  static wdRenderData::Category SimpleTransparent;
  static wdRenderData::Category SimpleForeground;
  static wdRenderData::Category Selection;
  static wdRenderData::Category GUI;
};

#define wdInvalidRenderDataCategory wdRenderData::Category()

struct WD_RENDERERCORE_DLL wdMsgExtractRenderData : public wdMessage
{
  WD_DECLARE_MESSAGE_TYPE(wdMsgExtractRenderData, wdMessage);

  const wdView* m_pView = nullptr;
  wdRenderData::Category m_OverrideCategory = wdInvalidRenderDataCategory;

  /// \brief Adds render data for the current view. This data can be cached depending on the specified caching behavior.
  /// Non-cached data is only valid for this frame. Cached data must be manually deleted using the wdRenderWorld::DeleteCachedRenderData
  /// function.
  void AddRenderData(const wdRenderData* pRenderData, wdRenderData::Category category, wdRenderData::Caching::Enum cachingBehavior);

private:
  friend class wdExtractor;

  struct Data
  {
    const wdRenderData* m_pRenderData = nullptr;
    wdUInt16 m_uiCategory = 0;
  };

  wdHybridArray<Data, 16> m_ExtractedRenderData;
  wdUInt32 m_uiNumCacheIfStatic = 0;
};

struct WD_RENDERERCORE_DLL wdMsgExtractOccluderData : public wdMessage
{
  WD_DECLARE_MESSAGE_TYPE(wdMsgExtractOccluderData, wdMessage);

  void AddOccluder(const wdRasterizerObject* pObject, const wdTransform& transform)
  {
    auto& d = m_ExtractedOccluderData.ExpandAndGetRef();
    d.m_pObject = pObject;
    d.m_Transform = transform;
  }

private:
  friend class wdRenderPipeline;

  struct Data
  {
    const wdRasterizerObject* m_pObject = nullptr;
    wdTransform m_Transform;
  };

  wdHybridArray<Data, 16> m_ExtractedOccluderData;
};

#include <RendererCore/Pipeline/Implementation/RenderData_inl.h>
