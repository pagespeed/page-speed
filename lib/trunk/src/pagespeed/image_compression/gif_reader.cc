/**
 * Copyright 2009 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Author: Bryan McQuade

#include "pagespeed/image_compression/gif_reader.h"

extern "C" {
#ifdef USE_SYSTEM_LIBPNG
#include "png.h"  // NOLINT
#else
#include "third_party/libpng/png.h"
#endif
#include "third_party/optipng/lib/pngxtern/gif/gifinput.h"
#include "third_party/optipng/lib/pngxtern/pngxtern.h"
}

namespace pagespeed {

namespace image_compression {

GifReader::~GifReader() {
}

bool GifReader::ReadPng(const std::string& body,
                        png_structp png_ptr,
                        png_infop info_ptr) {
  png_bytep data = reinterpret_cast<png_bytep>(const_cast<char*>(body.data()));
  GIFInput input;
  input.buf = data;
  input.len = body.length();
  input.pos = 0;

  // When positive, the return value of pngx_read_gif indicates the
  // number of frames in the GIF (1 for non-animated GIFs).
  const int read_result = pngx_read_gif(png_ptr, info_ptr, &input);
  if (read_result == 1) {
    return true;
  }

  if (read_result > 1) {
    LOG(INFO) << "Unable to convert animated GIF to PNG.";
    return false;
  }

  LOG(INFO) << "Failed to convert GIF to PNG.";
  return false;
}

}  // namespace image_compression

}  // namespace pagespeed
