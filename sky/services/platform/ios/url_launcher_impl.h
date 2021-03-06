// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SKY_SERVICES_PLATFORM_IOS_URL_LAUNCHER_IMPL_H_
#define SKY_SERVICES_PLATFORM_IOS_URL_LAUNCHER_IMPL_H_

#include "base/macros.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "sky/services/platform/url_launcher.mojom.h"

namespace flutter {
namespace platform {

class URLLauncherImpl : public URLLauncher {
 public:
  explicit URLLauncherImpl(mojo::InterfaceRequest<URLLauncher> request);

  ~URLLauncherImpl() override;

  void Launch(const mojo::String& uriString,
              const LaunchCallback& callback) override;

 private:
  mojo::StrongBinding<URLLauncher> binding_;

  DISALLOW_COPY_AND_ASSIGN(URLLauncherImpl);
};

}  // namespace platform
}  // namespace flutter

#endif  // SKY_SERVICES_PLATFORM_IOS_URL_LAUNCHER_IMPL_H_
