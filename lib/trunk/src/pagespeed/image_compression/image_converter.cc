/**
 * Copyright 2011 Google Inc.
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

// Author: Satyanarayana Manyam

#include "pagespeed/image_compression/image_converter.h"

#include <string>

#include "base/logging.h"

#include "pagespeed/image_compression/jpeg_optimizer.h"

namespace {
// In some cases, converting a PNG to JPEG results in a smaller file. This is at
// the cost of switching from lossless to lossy, so we require that the savings are
// substantial before in order to do the conversion. We choose 80% size reduction
// as the minimum before we switch a PNG to JPEG.
const double kMinJpegSavingsRatio = 0.8;
}  // namespace

namespace pagespeed {

namespace image_compression {

bool ImageConverter::ConvertImage(
    ScanlineReaderInterface& reader,
    ScanlineWriterInterface& writer) {
  void* scan_row;
  while(reader.HasMoreScanLines()) {
    if (!reader.ReadNextScanline(&scan_row)) {
      return false;
    }
    if (!writer.WriteNextScanline(scan_row)) {
      return false;
    }
  }

  if(!writer.FinalizeWrite()) {
    return false;
  }

  return true;
}

bool ImageConverter::OptimizePngOrConvertToJpeg(
    PngReaderInterface& png_struct_reader, const std::string& in,
    const JpegCompressionOptions& options, std::string* out, bool* is_out_png) {
  DCHECK(out->empty());
  out->clear();

  // Initialize the reader.
  PngScanlineReader png_reader;

  // Since JPEG only support 8 bits/channels, we need convert PNG
  // having 1,2,4,16 bits/channel to 8 bits/channel.
  //   -PNG_TRANSFORM_EXPAND expands 1,2 and 4 bit channels to 8 bit channels
  //   -PNG_TRANSFORM_STRIP_16 will strip 16 bit channels to get 8 bit/channel
  png_reader.set_transform(
      PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_STRIP_16);

  // Configure png reader error handlers.
  if (setjmp(*png_reader.GetJmpBuf())) {
    return false;
  }

  if (!png_reader.InitializeRead(png_struct_reader, in)) {
    return false;
  }

  // Try converting if the image is opaque.
  bool jpeg_success = false;
  size_t width = png_reader.GetImageWidth();
  size_t height = png_reader.GetImageHeight();
  PixelFormat format = png_reader.GetPixelFormat();

  if (height > 0 && width > 0 && format != UNSUPPORTED) {
    JpegScanlineWriter jpeg_writer;

    // libjpeg's error handling mechanism requires that longjmp be used
    // to get control after an error.
    jmp_buf env;
    if (setjmp(env)) {
      // This code is run only when libjpeg hit an error, and called
      // longjmp(env).
      jpeg_writer.AbortWrite();
    } else {
      jpeg_writer.SetJmpBufEnv(&env);
      if (jpeg_writer.Init(width, height, format)) {
        jpeg_writer.SetJpegCompressParams(options);
        jpeg_writer.InitializeWrite(out);
        jpeg_success = ConvertImage(png_reader, jpeg_writer);
      }
    }
  }

  // Try Optimizing the PNG.
  // TODO(satyanarayana): Try reusing the PNG structs for png->jpeg and optimize
  // png operations.
  std::string optimized_png_out;
  bool png_success = PngOptimizer::OptimizePngBestCompression(
      png_struct_reader, in, &optimized_png_out);

  // Consider using jpeg's only if it gives substantial amount of byte savings.
  if (png_success &&
      (!jpeg_success ||
       out->size() > kMinJpegSavingsRatio * optimized_png_out.size())) {
    out->clear();
    out->assign(optimized_png_out);
    *is_out_png = true;
  } else {
    *is_out_png = false;
  }

  return jpeg_success || png_success;
}

}  // namespace image_compression

}  // namespace pagespeed
