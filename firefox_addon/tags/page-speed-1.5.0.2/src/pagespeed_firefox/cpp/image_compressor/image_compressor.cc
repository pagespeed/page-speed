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

#ifdef PAGESPEED_GYP_BUILD
#include <fstream>

#include "pagespeed/image_compression/jpeg_optimizer.h"
#include "pagespeed/image_compression/png_optimizer.h"
#else
#include "jpeg_optimizer.h"
#include "png_optimizer.h"
#endif

NS_IMPL_ISUPPORTS1(pagespeed::ImageCompressor, IImageCompressor)

namespace {

#ifdef PAGESPEED_GYP_BUILD
enum ImageType {
  JPEG,
  PNG,
};

// Build the optimized image and write it to a file.
// return true on success
bool OptimizeImage(const char *in_filename,
                   const char *out_filename,
                   ImageType type) {
  std::ifstream in(in_filename, std::ios::in | std::ios::binary);
  if (!in) {
    fprintf(stderr, "Could not read input from %s\n", in_filename);
    return false;
  }

  in.seekg (0, std::ios::end);
  int length = in.tellg();
  in.seekg (0, std::ios::beg);

  char* buffer = new char[length];
  in.read(buffer, length);
  in.close();

  std::string original(buffer, length);
  delete[] buffer;

  std::string compressed;
  if (type == PNG) {
    pagespeed::image_compression::PngReader reader;
    if (!pagespeed::image_compression::PngOptimizer::OptimizePng(
            reader, original, &compressed)) {
      return false;
    }
  } else if (type == JPEG) {
    if (!pagespeed::image_compression::OptimizeJpeg(original, &compressed)) {
      return false;
    }
  }

  std::ofstream out(out_filename, std::ios::out | std::ios::binary);
  if (!out) {
    fprintf(stderr, "Could open %s for write.\n", out_filename);
    return false;
  }

  out.write(compressed.data(), compressed.length());
  out.close();

  return true;
}

#endif  // PAGESPEED_GYP_BUILD

}  // namespace

namespace pagespeed {

ImageCompressor::ImageCompressor() {
}

ImageCompressor::~ImageCompressor() {
}

NS_IMETHODIMP ImageCompressor::CompressToPng(
    const char *infile, const char *outfile) {
#ifdef PAGESPEED_GYP_BUILD
  if (!OptimizeImage(infile, outfile, PNG)) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
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
#ifdef PAGESPEED_GYP_BUILD
  if (!OptimizeImage(infile, outfile, JPEG)) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
#else
  JpegOptimizer jpeg_optimizer;

  if (!jpeg_optimizer.Initialize()) {
    return NS_ERROR_FAILURE;
  }

  bool result = jpeg_optimizer.CreateOptimizedJpeg(infile, outfile);

  if (!jpeg_optimizer.Finalize()) {
    return NS_ERROR_FAILURE;
  }

  return result ? NS_OK : NS_ERROR_FAILURE;
#endif
}

}  // namespace pagespeed
