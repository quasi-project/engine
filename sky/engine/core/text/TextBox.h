// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SKY_ENGINE_CORE_TEXT_TEXT_BOX_H_
#define SKY_ENGINE_CORE_TEXT_TEXT_BOX_H_

#include "dart/runtime/include/dart_api.h"
#include "lib/tonic/converter/dart_converter.h"
#include "sky/engine/platform/text/TextDirection.h"
#include "third_party/skia/include/core/SkRect.h"

namespace blink {

class TextBox {
 public:
  TextBox() : is_null(true) {}
  TextBox(SkRect r, TextDirection direction)
      : sk_rect(std::move(r)), direction(direction), is_null(false) {}

  SkRect sk_rect;
  TextDirection direction;
  bool is_null;
};

}  // namespace blink

namespace tonic {

template <>
struct DartConverter<blink::TextBox> {
  static Dart_Handle ToDart(const blink::TextBox& val);
};

}  // namespace tonic

#endif  // SKY_ENGINE_CORE_TEXT_TEXT_BOX_H_
