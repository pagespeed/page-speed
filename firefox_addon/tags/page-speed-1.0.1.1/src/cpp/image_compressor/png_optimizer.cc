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

#include "png_optimizer.h"

#include <string.h>  // for memset

#include "zlib.h"

namespace {

// Callbacks required by optipng.
void Printf(const char *fmt, ...) {}
void PrintCntrl(int cntrl_code) {}
void Progress(unsigned long num, unsigned long denom) {}
void Panic(const char *msg) {}

}  // namespace

namespace pagespeed {

PngOptimizer::PngOptimizer() {
  // Configure the optipng "UI" callbacks.
  ui_.printf_fn = &Printf;
  ui_.print_cntrl_fn = &PrintCntrl;
  ui_.progress_fn = &Progress;
  ui_.panic_fn = &Panic;
}

PngOptimizer::~PngOptimizer() {
}

bool PngOptimizer::Initialize() {
  // Configure the optipng options.
  memset(&options_, 0, sizeof(opng_options));

  // Preserve the interlace mode of the input file.
  options_.interlace = -1;

  // Choose compression options that result in a reasonable
  // runtime/compression tradeoff. Note that setting optim_level = 7
  // for a brute-force search of the compression space is far too
  // expensive. Tests shows that the following subset of options
  // performs nearly as well, at a fraction (10% of the brute-force
  // runtime).

  // Use best possible compression.
  BITSET_SET(options_.compr_level_set, Z_BEST_COMPRESSION);

  // Use the default memory level.
  BITSET_SET(options_.mem_level_set, 8);  // default zlib memory level

  // We use the default, filtered, and rle zlib strategies. See the
  // zlib documentation for a description of zlib strategies.
  BITSET_SET(options_.strategy_set, Z_DEFAULT_STRATEGY);
  BITSET_SET(options_.strategy_set, Z_FILTERED);
  BITSET_SET(options_.strategy_set, Z_RLE);

  // We disable filters, since they rarely reduce the size of the
  // image when combined with zlib compression.
  BITSET_SET(options_.filter_set, 0);  // no filter

  // Force overwriting if the output file is present.
  options_.force = 1;

  if (opng_initialize(&options_, &ui_) != 0) {
    return false;
  }

  return true;
}

bool PngOptimizer::Finalize() {
  if (opng_finalize() != 0) {
    return false;
  }

  return true;
}

bool PngOptimizer::CreateOptimizedPng(const char *infile, const char *outfile) {
  // opng_options.out_name expects a non-const char*, even though it
  // does not modify the contents of the character buffer. Thus, we
  // cast away constness to interoperate with the third-party optipng
  // API.
  options_.out_name = const_cast<char *>(outfile);
  int result = opng_optimize(infile);
  options_.out_name = NULL;
  if (result != 0) {
    return false;
  }

  return true;
}

}  // namespace pagespeed
