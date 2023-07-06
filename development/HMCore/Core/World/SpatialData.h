#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Strings/HashedString.h>

struct wdSpatialData
{
  struct Flags
  {
    using StorageType = wdUInt8;

    enum Enum
    {
      None = 0,
      FrequentChanges = WD_BIT(0), ///< Indicates that objects in this category change their bounds frequently. Spatial System implementations can use that as hint for internal optimizations.

      Default = None
    };

    struct Bits
    {
      StorageType FrequentUpdates : 1;
    };
  };

  struct Category
  {
    WD_ALWAYS_INLINE Category()
      : m_uiValue(wdInvalidIndex)
    {
    }

    WD_ALWAYS_INLINE explicit Category(wdUInt32 uiValue)
      : m_uiValue(uiValue)
    {
    }

    WD_ALWAYS_INLINE bool operator==(const Category& other) const { return m_uiValue == other.m_uiValue; }
    WD_ALWAYS_INLINE bool operator!=(const Category& other) const { return m_uiValue != other.m_uiValue; }

    wdUInt32 m_uiValue;

    WD_ALWAYS_INLINE wdUInt32 GetBitmask() const { return m_uiValue != wdInvalidIndex ? static_cast<wdUInt32>(WD_BIT(m_uiValue)) : 0; }
  };

  /// \brief Registers a spatial data category under the given name.
  ///
  /// If the same category was already registered before, it returns that instead.
  /// Asserts that there are no more than 32 unique categories.
  WD_CORE_DLL static Category RegisterCategory(wdStringView sCategoryName, const wdBitflags<Flags>& flags);

  /// \brief Returns either an existing category with the given name or wdInvalidSpatialDataCategory.
  WD_CORE_DLL static Category FindCategory(wdStringView sCategoryName);

  /// \brief Returns the flags for the given category.
  WD_CORE_DLL static const wdBitflags<Flags>& GetCategoryFlags(Category category);

private:
  struct CategoryData
  {
    wdHashedString m_sName;
    wdBitflags<Flags> m_Flags;
  };

  static wdHybridArray<wdSpatialData::CategoryData, 32>& GetCategoryData();
};

struct WD_CORE_DLL wdDefaultSpatialDataCategories
{
  static wdSpatialData::Category RenderStatic;
  static wdSpatialData::Category RenderDynamic;
  static wdSpatialData::Category OcclusionStatic;
  static wdSpatialData::Category OcclusionDynamic;
};

/// \brief When an object is 'seen' by a view and thus tagged as 'visible', this enum describes what kind of observer triggered this.
///
/// This is used to determine how important certain updates, such as animations, are to execute.
/// E.g. when a 'shadow view' or 'reflection view' is the only thing that observes an object, animations / particle effects and so on,
/// can be updated less frequently.
enum class wdVisibilityState : wdUInt8
{
  Invisible = 0, ///< The object isn't visible to any view.
  Indirect = 1,  ///< The object is seen by a view that only indirectly makes the object visible (shadow / reflection / render target).
  Direct = 2,    ///< The object is seen directly by a main view and therefore it needs to be updated at maximum frequency.
};

#define wdInvalidSpatialDataCategory wdSpatialData::Category()
