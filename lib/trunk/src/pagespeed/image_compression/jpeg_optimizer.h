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

// Author: Bryan McQuade, Matthew Steele

#ifndef JPEG_OPTIMIZER_H_
#define JPEG_OPTIMIZER_H_

#include <string>

#include "base/basictypes.h"

extern "C" {
#include "jpeglib.h"
}

namespace pagespeed {

namespace image_compression {

class JpegOptimizer {
 public:
  JpegOptimizer();
  ~JpegOptimizer();

  static bool OptimizeJpeg(const std::string &original,
                           std::string *compressed);

  // Take the given input file and losslessly compress it by removing
  // all unnecessary chunks.  If this function fails (returns false),
  // it can be called again.
  // @return true on success, false on failure.
  bool CreateOptimizedJpeg(const std::string &original,
                           std::string *compressed);

 private:
  bool DoCreateOptimizedJpeg(const std::string &original,
                             std::string *compressed);

 private:
  // Structures for jpeg decompression
  jpeg_decompress_struct jpeg_decompress_;
  jpeg_error_mgr decompress_error_;

  // Structures for jpeg compression.
  jpeg_compress_struct jpeg_compress_;
  jpeg_error_mgr compress_error_;

  DISALLOW_COPY_AND_ASSIGN(JpegOptimizer);
};

}  // namespace image_compression

}  // namespace pagespeed

#endif  // JPEG_OPTIMIZER_H_
