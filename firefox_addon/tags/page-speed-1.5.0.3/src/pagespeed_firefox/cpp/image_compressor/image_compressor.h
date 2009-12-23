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
//
// ImageCompressor losslessly compresses PNG, GIF, and JPG images.

#ifndef IMAGE_COMPRESSOR_H_
#define IMAGE_COMPRESSOR_H_

#include "IImageCompressor.h"

#include "base/basictypes.h"

namespace pagespeed {

class ImageCompressor : public IImageCompressor {
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_IIMAGECOMPRESSOR

  ImageCompressor();

 private:
  // Classes that implement xpcom interfaces should have private
  // destructors. xpcom uses ref counts for memory management; it is
  // illegal to use any other means to do memory management on these
  // objects.
  ~ImageCompressor();

  DISALLOW_COPY_AND_ASSIGN(ImageCompressor);
};

}  // namespace pagespeed

#endif  // IMAGE_COMPRESSOR_H_
