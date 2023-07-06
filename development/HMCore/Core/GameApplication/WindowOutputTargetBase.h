#pragma once

#include <Core/CoreDLL.h>

class wdImage;

/// \brief Base class for window output targets
///
/// A window output target is usually tied tightly to a window (\sa wdWindowBase) and represents the
/// graphics APIs side of the render output.
/// E.g. in a DirectX implementation this would be a swapchain.
///
/// This interface provides the high level functionality that is needed by wdGameApplication to work with
/// the render output.
class WD_CORE_DLL wdWindowOutputTargetBase
{
public:
  virtual ~wdWindowOutputTargetBase() = default;
  virtual void Present(bool bEnableVSync) = 0;
  virtual wdResult CaptureImage(wdImage& out_image) = 0;
};
