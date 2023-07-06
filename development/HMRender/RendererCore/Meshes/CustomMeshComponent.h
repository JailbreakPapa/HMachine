#pragma once

#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererCore/Pipeline/Renderer.h>

using wdDynamicMeshBufferResourceHandle = wdTypedResourceHandle<class wdDynamicMeshBufferResource>;
using wdCustomMeshComponentManager = wdComponentManager<class wdCustomMeshComponent, wdBlockStorageType::Compact>;

/// \brief This component is used to render custom geometry.
///
/// Sometimes game code needs to build geometry on the fly to visualize dynamic things.
/// The wdDynamicMeshBufferResource is an easy to use resource to build geometry and change it frequently.
/// This component takes such a resource and takes care of rendering it.
/// The same resource can be set on multiple components to instantiate it in different locations.
class WD_RENDERERCORE_DLL wdCustomMeshComponent : public wdRenderComponent
{
  WD_DECLARE_COMPONENT_TYPE(wdCustomMeshComponent, wdRenderComponent, wdCustomMeshComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // wdComponent

public:
  virtual void SerializeComponent(wdWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(wdWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // wdRenderComponent

public:
  virtual wdResult GetLocalBounds(wdBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, wdMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // wdCustomMeshComponent

public:
  wdCustomMeshComponent();
  ~wdCustomMeshComponent();

  /// \brief Creates a new dynamic mesh buffer.
  ///
  /// The new buffer can hold the given number of vertices and indices (either 16 bit or 32 bit).
  wdDynamicMeshBufferResourceHandle CreateMeshResource(wdGALPrimitiveTopology::Enum topology, wdUInt32 uiMaxVertices, wdUInt32 uiMaxPrimitives, wdGALIndexType::Enum indexType);

  /// \brief Returns the currently set mesh resource.
  wdDynamicMeshBufferResourceHandle GetMeshResource() const { return m_hDynamicMesh; }

  /// \brief Sets which mesh buffer to use.
  ///
  /// This can be used to have multiple wdCustomMeshComponent's reference the same mesh buffer,
  /// such that the object gets instanced in different locations.
  void SetMeshResource(const wdDynamicMeshBufferResourceHandle& hMesh);

  /// \brief Configures the component to render only a subset of the primitives in the mesh buffer.
  void SetUsePrimitiveRange(wdUInt32 uiFirstPrimitive = 0, wdUInt32 uiNumPrimitives = wdMath::MaxValue<wdUInt32>());

  /// \brief Sets the bounds that are used for culling.
  ///
  /// Note: It is very important that this is called whenever the mesh buffer is modified and the size of
  /// the mesh has changed, otherwise the object might not appear or be culled incorrectly.
  void SetBounds(const wdBoundingBoxSphere& bounds);

  /// \brief Sets the material for rendering.
  void SetMaterial(const wdMaterialResourceHandle& hMaterial);

  /// \brief Returns the material that is used for rendering.
  wdMaterialResourceHandle GetMaterial() const;

  void SetMaterialFile(const char* szMaterial); // [ property ]
  const char* GetMaterialFile() const;          // [ property ]

  /// \brief Sets a specific render data category used for rendering.
  ///
  /// Typically it is not necessary to change this, but it can be used to render the object in a
  /// certain render pass.
  WD_ALWAYS_INLINE void SetRenderDataCategory(wdRenderData::Category category) { m_RenderDataCategory = category; }

  /// \brief Sets the mesh instance color.
  void SetColor(const wdColor& color); // [ property ]

  /// \brief Returns the mesh instance color.
  const wdColor& GetColor() const; // [ property ]

  void OnMsgSetMeshMaterial(wdMsgSetMeshMaterial& ref_msg); // [ msg handler ]
  void OnMsgSetColor(wdMsgSetColor& ref_msg);               // [ msg handler ]

protected:
  void OnMsgExtractRenderData(wdMsgExtractRenderData& msg) const;

  wdRenderData::Category m_RenderDataCategory = wdInvalidRenderDataCategory;
  wdMaterialResourceHandle m_hMaterial;
  wdColor m_Color = wdColor::White;
  wdUInt32 m_uiFirstPrimitive = 0;
  wdUInt32 m_uiNumPrimitives = 0xFFFFFFFF;
  wdBoundingBoxSphere m_Bounds;

  wdDynamicMeshBufferResourceHandle m_hDynamicMesh;

  virtual void OnActivated() override;
};

/// \brief Temporary data used to feed the wdCustomMeshRenderer.
class WD_RENDERERCORE_DLL wdCustomMeshRenderData : public wdRenderData
{
  WD_ADD_DYNAMIC_REFLECTION(wdCustomMeshRenderData, wdRenderData);

public:
  virtual void FillBatchIdAndSortingKey();

  wdDynamicMeshBufferResourceHandle m_hMesh;
  wdMaterialResourceHandle m_hMaterial;
  wdColor m_Color = wdColor::White;

  wdUInt32 m_uiFlipWinding : 1;
  wdUInt32 m_uiUniformScale : 1;

  wdUInt32 m_uiFirstPrimitive = 0;
  wdUInt32 m_uiNumPrimitives = 0xFFFFFFFF;

  wdUInt32 m_uiUniqueID = 0;
};

/// \brief A renderer that handles all wdCustomMeshRenderData.
class WD_RENDERERCORE_DLL wdCustomMeshRenderer : public wdRenderer
{
  WD_ADD_DYNAMIC_REFLECTION(wdCustomMeshRenderer, wdRenderer);
  WD_DISALLOW_COPY_AND_ASSIGN(wdCustomMeshRenderer);

public:
  wdCustomMeshRenderer();
  ~wdCustomMeshRenderer();

  virtual void GetSupportedRenderDataCategories(wdHybridArray<wdRenderData::Category, 8>& ref_categories) const override;
  virtual void GetSupportedRenderDataTypes(wdHybridArray<const wdRTTI*, 8>& ref_types) const override;
  virtual void RenderBatch(const wdRenderViewContext& renderContext, const wdRenderPipelinePass* pPass, const wdRenderDataBatch& batch) const override;
};
