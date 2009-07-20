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

#ifndef JPEG_OPTIMIZER_H_
#define JPEG_OPTIMIZER_H_

#include <stdio.h>  // libjpeg forward-references FILE, so we're
                    // forced to include stdio here.

#include "base/macros.h"

extern "C" {
#include "jpeglib.h"
}

namespace pagespeed {

class JpegOptimizer {
 public:
  JpegOptimizer();
  ~JpegOptimizer();

  // Initialize the optimizer. Can be called multiple times. Must be
  // paired with a call to Finalize().
  // @return true on success, false on failure.
  bool Initialize();

  // Take the given input file and losslessly compress it by removing
  // all unnecessary chunks.  If this function fails (returns false),
  // it can be called again (without calling Initialize() again).
  // @return true on success, false on failure.
  bool CreateOptimizedJpeg(const char *infile, const char *outfile);

  // Finalize the optimizer. Can be called multiple times. Must be
  // paired with a call to Initialize().
  // @return true on success, false on failure.
  bool Finalize();

 private:
  bool DoCreateOptimizedJpeg(const char *infile, const char *outfile);

 private:
  // Structures for jpeg decompression
  jpeg_decompress_struct jpeg_decompress_;
  jpeg_error_mgr decompress_error_;

  // Structures for jpeg compression.
  jpeg_compress_struct jpeg_compress_;
  jpeg_error_mgr compress_error_;

  DISALLOW_COPY_AND_ASSIGN(JpegOptimizer);
};

}  // namespace pagespeed

#endif  // JPEG_OPTIMIZER_H_
