#pragma once

#include <Foundation/Containers/Deque.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Types/ArrayPtr.h>
#include <RendererCore/RendererCoreDLL.h>

class Rasterizer;
class wdRasterizerObject;
class wdColorLinearUB;
class wdCamera;
class wdSimdBBox;

class WD_RENDERERCORE_DLL wdRasterizerView final
{
  WD_DISALLOW_COPY_AND_ASSIGN(wdRasterizerView);

public:
  wdRasterizerView();
  ~wdRasterizerView();

  /// \brief Changes the resolution of the view. Has to be called at least once before starting to render anything.
  void SetResolution(wdUInt32 uiWidth, wdUInt32 uiHeight, float fAspectRatio);

  wdUInt32 GetResolutionX() const { return m_uiResolutionX; }
  wdUInt32 GetResolutionY() const { return m_uiResolutionY; }

  /// \brief Prepares the view to rasterize a new scene.
  void BeginScene();

  /// \brief Finishes rasterizing the scene. Visibility queries only work after this.
  void EndScene();

  /// \brief Writes an RGBA8 representation of the depth values to targetBuffer.
  ///
  /// The buffer must be large enough for the chosen resolution.
  void ReadBackFrame(wdArrayPtr<wdColorLinearUB> targetBuffer) const;

  /// \brief Sets the camera from which to extract the rendering position, direction and field-of-view.
  void SetCamera(const wdCamera* pCamera)
  {
    m_pCamera = pCamera;
  }

  /// \brief Adds an object as an occluder to the scene. Once all occluders have been rasterized, visibility queries can be done.
  void AddObject(const wdRasterizerObject* pObject, const wdTransform& transform)
  {
    auto& inst = m_Instances.ExpandAndGetRef();
    inst.m_pObject = pObject;
    inst.m_Transform = transform;
  }

  /// \brief Checks whether a box would be visible, or is fully occluded by the existing scene geometry.
  ///
  /// Note: This only works after EndScene().
  bool IsVisible(const wdSimdBBox& aabb) const;

  /// \brief Wether any occluder was actually added and also rasterized. If not, no need to do any visibility checks.
  bool HasRasterizedAnyOccluders() const
  {
    return m_bAnyOccludersRasterized;
  }

private:
  void SortObjectsFrontToBack();
  void RasterizeObjects(wdUInt32 uiMaxObjects);
  void UpdateViewProjectionMatrix();
  void ApplyModelViewProjectionMatrix(const wdTransform& modelTransform);

  bool m_bAnyOccludersRasterized = false;
  const wdCamera* m_pCamera = nullptr;
  wdUInt32 m_uiResolutionX = 0;
  wdUInt32 m_uiResolutionY = 0;
  float m_fAspectRation = 1.0f;
  wdUniquePtr<Rasterizer> m_pRasterizer;

  struct Instance
  {
    wdTransform m_Transform;
    const wdRasterizerObject* m_pObject;
  };

  wdDeque<Instance> m_Instances;
  wdMat4 m_mViewProjection;
};

class wdRasterizerViewPool
{
public:
  wdRasterizerView* GetRasterizerView(wdUInt32 uiWidth, wdUInt32 uiHeight, float fAspectRatio);
  void ReturnRasterizerView(wdRasterizerView* pView);

private:
  struct PoolEntry
  {
    bool m_bInUse = false;
    wdRasterizerView m_RasterizerView;
  };

  wdMutex m_Mutex;
  wdDeque<PoolEntry> m_Entries;
};
