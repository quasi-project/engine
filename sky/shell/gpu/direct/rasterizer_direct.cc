// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sky/shell/gpu/direct/rasterizer_direct.h"

#include "base/trace_event/trace_event.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "sky/engine/wtf/PassRefPtr.h"
#include "sky/engine/wtf/RefPtr.h"
#include "sky/shell/gpu/picture_serializer.h"
#include "sky/shell/platform_view.h"
#include "sky/shell/shell.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkPicture.h"

namespace sky {
namespace shell {

RasterizerDirect::RasterizerDirect()
    : binding_(this), platform_view_(nullptr), weak_factory_(this) {}

RasterizerDirect::~RasterizerDirect() {
  weak_factory_.InvalidateWeakPtrs();
  Shell::Shared().PurgeRasterizers();
}

// Implementation of declaration in sky/shell/rasterizer.h.
std::unique_ptr<Rasterizer> Rasterizer::Create() {
  return std::unique_ptr<Rasterizer>(new RasterizerDirect());
}

// sky::shell::Rasterizer override.
base::WeakPtr<Rasterizer> RasterizerDirect::GetWeakRasterizerPtr() {
  return weak_factory_.GetWeakPtr();
}

// sky::shell::Rasterizer override.
void RasterizerDirect::ConnectToRasterizer(
    mojo::InterfaceRequest<rasterizer::Rasterizer> request) {
  binding_.Bind(request.Pass());

  Shell::Shared().AddRasterizer(GetWeakRasterizerPtr());
}

// sky::shell::Rasterizer override.
void RasterizerDirect::Setup(PlatformView* platform_view,
                             base::Closure continuation,
                             base::WaitableEvent* setup_completion_event) {
  CHECK(platform_view) << "Must be able to acquire the view.";

  // The context needs to be made current before the GrGL interface can be
  // setup.
  bool success = platform_view->ContextMakeCurrent();
  if (success) {
    success = ganesh_canvas_.SetupGrGLInterface();
    if (!success)
      LOG(ERROR) << "Could not create the GL interface";
  } else {
    LOG(ERROR) << "Could not make the context current for initial GL setup";
  }

  if (success) {
    platform_view_ = platform_view;
  } else {
    LOG(ERROR) << "WARNING: Flutter will be unable to render to the display";
  }

  continuation.Run();

  setup_completion_event->Signal();
}

// sky::shell::Rasterizer override.
void RasterizerDirect::Teardown(
    base::WaitableEvent* teardown_completion_event) {
  platform_view_ = nullptr;
  last_layer_tree_.reset();
  compositor_context_.OnGrContextDestroyed();
  teardown_completion_event->Signal();
}

// sky::shell::Rasterizer override.
flow::LayerTree* RasterizerDirect::GetLastLayerTree() {
  return last_layer_tree_.get();
}

void RasterizerDirect::Draw(uint64_t layer_tree_ptr,
                            const DrawCallback& callback) {
  TRACE_EVENT0("flutter", "RasterizerDirect::Draw");

  if (platform_view_ == nullptr) {
    callback.Run();
    return;
  }

  std::unique_ptr<flow::LayerTree> layer_tree(
      reinterpret_cast<flow::LayerTree*>(layer_tree_ptr));

  SkISize size = layer_tree->frame_size();
  if (platform_view_->GetSize() != size) {
    platform_view_->Resize(size);
  }

  if (!platform_view_->ContextMakeCurrent() || !layer_tree->root_layer()) {
    callback.Run();
    return;
  }

  // There is no way for the compositor to know how long the layer tree
  // construction took. Fortunately, the layer tree does. Grab that time
  // for instrumentation.
  compositor_context_.engine_time().SetLapTime(layer_tree->construction_time());

  {
    SkCanvas* canvas = ganesh_canvas_.GetCanvas(
        platform_view_->DefaultFramebuffer(), layer_tree->frame_size());
    flow::CompositorContext::ScopedFrame frame =
        compositor_context_.AcquireFrame(ganesh_canvas_.gr_context(), *canvas);
    canvas->clear(SK_ColorBLACK);
    layer_tree->Raster(frame);

    {
      TRACE_EVENT0("flutter", "SkCanvas::Flush");
      canvas->flush();
    }

    platform_view_->SwapBuffers();
  }

  // Trace to a file if necessary
  static const double kOneFrameDuration = 1e3 / 60.0;
  bool frameExceededThreshold = false;
  uint32_t thresholdInterval = layer_tree->rasterizer_tracing_threshold();
  if (thresholdInterval != 0 &&
      compositor_context_.frame_time().LastLap().ToMillisecondsF() >
          thresholdInterval * kOneFrameDuration) {
    // While rendering the last frame, if we exceeded the tracing threshold
    // specified in the layer tree, we force a trace to disk.
    frameExceededThreshold = true;
  }

  const auto& tracingController = Shell::Shared().tracing_controller();

  if (frameExceededThreshold || tracingController.picture_tracing_enabled()) {
    base::FilePath path = tracingController.PictureTracingPathForCurrentTime();

    SkPictureRecorder recoder;
    recoder.beginRecording(SkRect::MakeWH(size.width(), size.height()));

    {
      auto frame = compositor_context_.AcquireFrame(
          nullptr, *recoder.getRecordingCanvas(), false);
      layer_tree->Raster(frame);
    }

    sk_sp<SkPicture> picture = recoder.finishRecordingAsPicture();
    SerializePicture(path, picture.get());
  }

  callback.Run();

  last_layer_tree_ = std::move(layer_tree);
}

}  // namespace shell
}  // namespace sky
