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

#include "pagespeed/image_compression/png_optimizer.h"

#include <sstream>
#include <string>

extern "C" {
#include "third_party/libpng/png.h"
#include "third_party/optipng/src/opngreduc.h"
}

namespace {

void ReadPngFromStream(
    png_structp read_ptr, png_bytep data, png_size_t length) {
  std::istringstream& response_body =
      *reinterpret_cast<std::istringstream*>(read_ptr->io_ptr);
  response_body.read(reinterpret_cast<char*>(data), length);
}

void WritePngToString(
    png_structp write_ptr, png_bytep data, png_size_t length) {
  std::string& buffer = *reinterpret_cast<std::string*>(write_ptr->io_ptr);
  buffer.append(reinterpret_cast<char*>(data), length);
}

// no-op
void PngFlush(png_structp write_ptr) {}

}  // namespace

namespace pagespeed {

namespace image_compression {

PngOptimizer::PngOptimizer() {
  read_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  read_info_ptr = png_create_info_struct(read_ptr);
  write_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  write_info_ptr = png_create_info_struct(write_ptr);
}

PngOptimizer::~PngOptimizer() {
  png_destroy_read_struct(&read_ptr, &read_info_ptr, NULL);
  png_destroy_write_struct(&write_ptr, &write_info_ptr);
}

bool PngOptimizer::CreateOptimizedPng(const std::string& in, std::string* out) {
  // Configure error handlers.
  if (setjmp(read_ptr->jmpbuf)) {
    return false;
  }

  if (setjmp(write_ptr->jmpbuf)) {
    return false;
  }

  if (!ReadPng(in)) {
    return false;
  }

  // Perform all possible lossless image reductions
  // (e.g. RGB->palette, etc).
  opng_reduce_image(read_ptr, read_info_ptr, OPNG_REDUCE_ALL);

  // Copy the image data from the read structures to the write structures.
  CopyReadToWrite();

  // TODO: try a few different strategies and pick the best one.
  png_set_compression_level(write_ptr, Z_BEST_COMPRESSION);
  png_set_compression_mem_level(write_ptr, 8);
  png_set_compression_strategy(write_ptr, Z_DEFAULT_STRATEGY);
  png_set_filter(write_ptr, PNG_FILTER_TYPE_BASE, PNG_FILTER_NONE);
  png_set_compression_window_bits(write_ptr, 9);

  if (!WritePng(out)) {
    return false;
  }

  return true;
}

bool PngOptimizer::OptimizePng(const std::string& in, std::string* out) {
  PngOptimizer o;
  return o.CreateOptimizedPng(in, out);
}

bool PngOptimizer::ReadPng(const std::string& body) {
  // Wrap the resource's response body in an istringstream, and pass
  // a pointer to the istringstream as the user data to be received
  // by the PNG read function.
  std::istringstream body_stream(body, std::istringstream::in);
  png_set_read_fn(read_ptr, &body_stream, &ReadPngFromStream);
  png_read_png(read_ptr, read_info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

  if (!opng_validate_image(read_ptr, read_info_ptr)) {
    return false;
  }

  return true;
}

bool PngOptimizer::WritePng(std::string* buffer) {
  png_set_write_fn(write_ptr, buffer, &WritePngToString, &PngFlush);
  png_write_png(write_ptr, write_info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

  return true;
}

void PngOptimizer::CopyReadToWrite() {
  png_uint_32 width, height;
  int bit_depth, color_type, interlace_type, compression_type, filter_type;
  png_get_IHDR(read_ptr,
               read_info_ptr,
               &width,
               &height,
               &bit_depth,
               &color_type,
               &interlace_type,
               &compression_type,
               &filter_type);

  png_set_IHDR(write_ptr,
               write_info_ptr,
               width,
               height,
               bit_depth,
               color_type,
               interlace_type,
               compression_type,
               filter_type);

  png_bytepp row_pointers = png_get_rows(read_ptr, read_info_ptr);
  png_set_rows(write_ptr, write_info_ptr, row_pointers);

  png_colorp palette;
  int num_palette;
  if (png_get_PLTE(read_ptr, read_info_ptr, &palette, &num_palette) != 0) {
    png_set_PLTE(write_ptr,
                 write_info_ptr,
                 palette,
                 num_palette);
  }

  // Transparency is not considered metadata, although tRNS is
  // ancillary.  See the comment in opng_is_critical_chunk() above.
  png_bytep trans;
  int num_trans;
  png_color_16p trans_values;
  if (png_get_tRNS(read_ptr,
                   read_info_ptr,
                   &trans,
                   &num_trans, &
                   trans_values) != 0) {
    png_set_tRNS(write_ptr,
                 write_info_ptr,
                 trans,
                 num_trans,
                 trans_values);
  }

  double gamma;
  if (png_get_gAMA(read_ptr, read_info_ptr, &gamma) != 0) {
    png_set_gAMA(write_ptr, write_info_ptr, gamma);
  }

#if defined(PNG_bKGD_SUPPORTED) || defined(PNG_READ_BACKGROUND_SUPPORTED)
  png_color_16p background;
  if (png_get_bKGD(read_ptr, read_info_ptr, &background) != 0) {
    png_set_bKGD(write_ptr, write_info_ptr, background);
  }
#endif

#if defined(PNG_hIST_SUPPORTED)
  png_color_16p hist;
  if (png_get_hIST(read_ptr, read_info_ptr, &hist) != 0) {
    png_set_hIST(write_ptr, write_info_ptr, hist);
  }
#endif

#if defined(PNG_sBIT_SUPPORTED)
  png_color_8p sig_bit;
  if (png_get_sBIT(read_ptr, read_info_ptr, &sig_bit) != 0) {
    png_set_sBIT(write_ptr, write_info_ptr, sig_bit);
  }
#endif
}

}  // namespace image_compression

}  // namespace pagespeed
