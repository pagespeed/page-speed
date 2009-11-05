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

#include "image_compressor.h"

#include "jpeg_optimizer.h"
#ifndef PAGESPEED_DISABLE_PNG_OPTIMIZATION
#include "png_optimizer.h"
#endif

NS_IMPL_ISUPPORTS1(pagespeed::ImageCompressor, IImageCompressor)

namespace pagespeed {

ImageCompressor::ImageCompressor() {
}

ImageCompressor::~ImageCompressor() {
}

NS_IMETHODIMP ImageCompressor::CompressToPng(
    const char *infile, const char *outfile) {
#ifdef PAGESPEED_DISABLE_PNG_OPTIMIZATION
  return NS_ERROR_FAILURE;
#else
  PngOptimizer png_optimizer;

  if (!png_optimizer.Initialize()) {
    return NS_ERROR_FAILURE;
  }

  bool result = png_optimizer.CreateOptimizedPng(infile, outfile);

  if (!png_optimizer.Finalize()) {
    return NS_ERROR_FAILURE;
  }

  return result ? NS_OK : NS_ERROR_FAILURE;
#endif
}

NS_IMETHODIMP ImageCompressor::CompressJpeg(
    const char *infile, const char *outfile) {
  JpegOptimizer jpeg_optimizer;

  if (!jpeg_optimizer.Initialize()) {
    return NS_ERROR_FAILURE;
  }

  bool result = jpeg_optimizer.CreateOptimizedJpeg(infile, outfile);

  if (!jpeg_optimizer.Finalize()) {
    return NS_ERROR_FAILURE;
  }

  return result ? NS_OK : NS_ERROR_FAILURE;
}

}  // namespace pagespeed
