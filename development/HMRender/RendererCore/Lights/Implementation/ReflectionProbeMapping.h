#pragma once

#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>
#include <RendererFoundation/RendererFoundationDLL.h>

/// \brief Event generated on mapping changes.
/// \sa wdReflectionProbeMapping::m_Events
struct wdReflectionProbeMappingEvent
{
  enum class Type
  {
    ProbeMapped,          ///< The given probe was mapped to the atlas.
    ProbeUnmapped,        ///<  The given probe was unmapped from the atlas.
    ProbeUpdateRequested, ///< The given probe needs to be updated after which wdReflectionProbeMapping::ProbeUpdateFinished must be called.
  };

  wdReflectionProbeId m_Id;
  Type m_Type;
};

/// \brief This class creates a reflection probe atlas and controls the mapping of added probes to the available atlas indices.
class wdReflectionProbeMapping
{
public:
  /// \brief Creates a reflection probe atlas and mapping of the given size.
  /// \param uiAtlasSize How many probes the atlas can contain.
  wdReflectionProbeMapping(wdUInt32 uiAtlasSize);
  ~wdReflectionProbeMapping();

  /// \name Probe management
  ///@{

  /// \brief Adds a probe that will be considered for mapping into the atlas.
  void AddProbe(wdReflectionProbeId probe, wdBitflags<wdProbeFlags> flags);

  /// \brief Marks previously added probe as dirty and potentially changes its flags.
  void UpdateProbe(wdReflectionProbeId probe, wdBitflags<wdProbeFlags> flags);

  /// \brief Should be called once a requested wdReflectionProbeMappingEvent::Type::ProbeUpdateRequested event has been completed.
  /// \param probe The probe that has finished its update.
  void ProbeUpdateFinished(wdReflectionProbeId probe);

  /// \brief Removes a probe. If the probe was mapped, wdReflectionProbeMappingEvent::Type::ProbeUnmapped will be fired when calling this function.
  void RemoveProbe(wdReflectionProbeId probe);

  ///@}
  /// \name Render helpers
  ///@{

  /// \brief Returns the index at which a given probe is mapped.
  /// \param probe The probe that is being queried.
  /// \param bForExtraction If set, returns whether the index can be used for using the probe during rendering. If the probe was just mapped but not updated yet, -1 will be returned for bForExtraction = true but a valid index for bForExtraction = false so that the index can be rendered into.
  /// \return Returns the mapped index in the atlas or -1 of the probe is not mapped.
  wdInt32 GetReflectionIndex(wdReflectionProbeId probe, bool bForExtraction = false) const;

  /// \brief Returns the atlas texture.
  /// \return The texture handle of the cube map atlas.
  wdGALTextureHandle GetTexture() const { return m_hReflectionSpecularTexture; }

  ///@}
  /// \name Compute atlas mapping
  ///@{

  /// \brief Should be called in the PreExtraction phase. This will reset all probe weights.
  void PreExtraction();

  /// \brief Adds weight to a probe. Should be called during extraction of the probe. The mapping will map the probes with the highest weights in the atlas over time. This can be called multiple times in a frame for a probe if it is visible in multiple views. The maximum weight is then taken.
  void AddWeight(wdReflectionProbeId probe, float fPriority);

  /// \brief Should be called in the PostExtraction phase. This will compute the best probe mapping and potentially fire wdReflectionProbeMappingEvent events to map / unmap or request updates of probes.
  void PostExtraction();

  ///@}

public:
  wdEvent<const wdReflectionProbeMappingEvent&> m_Events;

private:
  struct wdProbeMappingFlags
  {
    using StorageType = wdUInt8;

    enum Enum
    {
      SkyLight = wdProbeFlags::SkyLight,
      HasCustomCubeMap = wdProbeFlags::HasCustomCubeMap,
      Sphere = wdProbeFlags::Sphere,
      Box = wdProbeFlags::Box,
      Dynamic = wdProbeFlags::Dynamic,
      Dirty = WD_BIT(5),
      Usable = WD_BIT(6),
      Default = 0
    };

    struct Bits
    {
      StorageType SkyLight : 1;
      StorageType HasCustomCubeMap : 1;
      StorageType Sphere : 1;
      StorageType Box : 1;
      StorageType Dynamic : 1;
      StorageType Dirty : 1;
      StorageType Usable : 1;
    };
  };

  //WD_DECLARE_FLAGS_OPERATORS(wdProbeMappingFlags);

  struct SortedProbes
  {
    WD_DECLARE_POD_TYPE();

    WD_ALWAYS_INLINE bool operator<(const SortedProbes& other) const
    {
      if (m_fPriority > other.m_fPriority) // we want to sort descending (higher priority first)
        return true;

      return m_uiIndex < other.m_uiIndex;
    }

    wdReflectionProbeId m_uiIndex;
    float m_fPriority = 0.0f;
  };

  struct ProbeDataInternal
  {
    wdBitflags<wdProbeMappingFlags> m_Flags;
    wdInt32 m_uiReflectionIndex = -1;
    float m_fPriority = 0.0f;
    wdReflectionProbeId m_id;
  };

private:
  void MapProbe(wdReflectionProbeId id, wdInt32 iReflectionIndex);
  void UnmapProbe(wdReflectionProbeId id);

private:
  wdDynamicArray<ProbeDataInternal> m_RegisteredProbes;
  wdReflectionProbeId m_SkyLight;

  wdUInt32 m_uiAtlasSize = 32;
  wdDynamicArray<wdReflectionProbeId> m_MappedCubes;

  // GPU Data
  wdGALTextureHandle m_hReflectionSpecularTexture;

  // Cleared every frame:
  wdDynamicArray<SortedProbes> m_SortedProbes; // All probes exiting in the scene, sorted by priority.
  wdDynamicArray<SortedProbes> m_ActiveProbes; // Probes that are currently mapped in the atlas.
  wdDynamicArray<wdInt32> m_UnusedProbeSlots;  // Probe slots are are currently unused in the atlas.
  wdDynamicArray<SortedProbes> m_AddProbes;    // Probes that should be added to the atlas
};
