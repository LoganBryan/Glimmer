#pragma once

#include <mutex>
#include <atomic>
#include <future>

class OpenXRSession
{
public:
	struct RenderLayerInfo
	{
		XrTime predictedDisplayTime;
		std::vector<XrCompositionLayerBaseHeader*> layers;
		XrCompositionLayerProjection layerProjection = { XR_TYPE_COMPOSITION_LAYER_PROJECTION };
		std::vector<XrCompositionLayerProjectionView> layerProjectionViews;
	};

public:
	virtual void CreateSession() = 0;
	virtual void DestroySession() = 0;
	virtual XrSession GetSession() = 0;
	virtual void PollSystemEvents() = 0;
	virtual void PollEvents() = 0;
	virtual void CreateReferenceSpace() = 0;
	virtual void DestroyReferenceSpace() = 0;
	virtual bool RenderLayer(RenderLayerInfo& renderlayerInfo) = 0;
	virtual void RenderDesktopWindow() = 0;
	virtual void RenderFrame() = 0;

};
