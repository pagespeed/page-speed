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

#ifndef PNG_OPTIMIZER_H_
#define PNG_OPTIMIZER_H_

#include "optipng.h"

#include "base/macros.h"

namespace pagespeed {

class PngOptimizer {
 public:
  PngOptimizer();
  ~PngOptimizer();

  // Initialize the optimizer. Can be called multiple times. Must be
  // paired with a call to Initialize().
  // @return true on success, false on failure.
  bool Initialize();

  // Take the given input file and losslessly compress it by removing
  // all unnecessary chunks, and by choosing an optimal PNG encoding.
  // @return true on success, false on failure.
  bool CreateOptimizedPng(const char *infile, const char *outfile);

  // Finalize the optimizer. Can be called multiple times. Must be
  // paired with a call to Initialize().
  // @return true on success, false on failure.
  bool Finalize();

 private:
  opng_options options_;
  opng_ui ui_;

  DISALLOW_COPY_AND_ASSIGN(PngOptimizer);
};

}  // namespace pagespeed

#endif  // PNG_OPTIMIZER_H_
