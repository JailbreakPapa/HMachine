#pragma once

#include <Core/Graphics/Camera.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <RendererCore/Pipeline/ViewRenderMode.h>
#include <RendererFoundation/Device/SwapChain.h>

/// \brief Holds view data like the viewport, view and projection matrices
struct WD_RENDERERCORE_DLL wdViewData
{
  wdViewData()
  {
    m_ViewPortRect = wdRectFloat(0.0f, 0.0f);
    m_ViewRenderMode = wdViewRenderMode::None;

    for (int i = 0; i < 2; ++i)
    {
      m_ViewMatrix[i].SetIdentity();
      m_InverseViewMatrix[i].SetIdentity();
      m_ProjectionMatrix[i].SetIdentity();
      m_InverseProjectionMatrix[i].SetIdentity();
      m_ViewProjectionMatrix[i].SetIdentity();
      m_InverseViewProjectionMatrix[i].SetIdentity();
    }
  }

  wdGALRenderTargets m_renderTargets;
  wdGALSwapChainHandle m_hSwapChain;
  wdRectFloat m_ViewPortRect;
  wdEnum<wdViewRenderMode> m_ViewRenderMode;
  wdEnum<wdCameraUsageHint> m_CameraUsageHint;

  // Each matrix is there for both left and right camera lens.
  wdMat4 m_ViewMatrix[2];
  wdMat4 m_InverseViewMatrix[2];
  wdMat4 m_ProjectionMatrix[2];
  wdMat4 m_InverseProjectionMatrix[2];
  wdMat4 m_ViewProjectionMatrix[2];
  wdMat4 m_InverseViewProjectionMatrix[2];

  /// \brief Returns the start position and direction (in world space) of the picking ray through the screen position in this view.
  ///
  /// fScreenPosX and fScreenPosY are expected to be in [0; 1] range (normalized pixel coordinates).
  /// If no ray can be computed, WD_FAILURE is returned.
  wdResult ComputePickingRay(
    float fScreenPosX, float fScreenPosY, wdVec3& out_vRayStartPos, wdVec3& out_vRayDir, wdCameraEye eye = wdCameraEye::Left) const
  {
    wdVec3 vScreenPos;
    vScreenPos.x = fScreenPosX;
    vScreenPos.y = 1.0f - fScreenPosY;
    vScreenPos.z = 0.0f;

    return wdGraphicsUtils::ConvertScreenPosToWorldPos(
      m_InverseViewProjectionMatrix[static_cast<int>(eye)], 0, 0, 1, 1, vScreenPos, out_vRayStartPos, &out_vRayDir);
  }

  wdResult ComputeScreenSpacePos(const wdVec3& vPoint, wdVec3& out_vScreenPos, wdCameraEye eye = wdCameraEye::Left) const
  {
    wdUInt32 x = (wdUInt32)m_ViewPortRect.x;
    wdUInt32 y = (wdUInt32)m_ViewPortRect.y;
    wdUInt32 w = (wdUInt32)m_ViewPortRect.width;
    wdUInt32 h = (wdUInt32)m_ViewPortRect.height;

    if (wdGraphicsUtils::ConvertWorldPosToScreenPos(m_ViewProjectionMatrix[static_cast<int>(eye)], x, y, w, h, vPoint, out_vScreenPos).Succeeded())
    {
      out_vScreenPos.y = m_ViewPortRect.height - out_vScreenPos.y;

      return WD_SUCCESS;
    }

    return WD_FAILURE;
  }
};
