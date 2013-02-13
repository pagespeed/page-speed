/*
 * Copyright 2013 Google Inc.
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

// Author: Huibao Lin

#ifndef THIRD_PARTY_PAGESPEED_SRC_PAGESPEED_IMAGE_COMPRESSION_IMAGE_RESIZER_H_
#define THIRD_PARTY_PAGESPEED_SRC_PAGESPEED_IMAGE_COMPRESSION_IMAGE_RESIZER_H_

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "pagespeed/image_compression/scanline_interface.h"

namespace pagespeed {

namespace image_compression {

class ResizeRow;
class ResizeCol;

// Class ScanlineResizer resizes an image, and outputs a scanline at a time.
// To use it, you need to provide an initialized image reader, which must have
// the type of ScanlineReaderInterface. The ScanlineResizer object will
// instruct the reader to fetch image required for the resized scanline.
//
// You can specify both the output width and height. The unit of both is pixel.
// If you want to preserve the aspect ratio of the input image, you can
// specify only of them, and leave the other one to 0.
//
// Currently, ScanlineResizer only supports shrinking.
//
class ScanlineResizer : public ScanlineReaderInterface {
 public:
  ScanlineResizer();
  virtual ~ScanlineResizer();

  // Initialize the resizer. You must initialize the reader before sending
  // it to Initialize(). You can set both the width and height for the output.
  // If you want to preserve the aspect ratio of the input image, you can
  // set only either the width or the height, and leave the other one to 0.
  //
  bool Initialize(ScanlineReaderInterface* reader,
                  size_t output_width,
                  size_t output_height);

  // Read the next available scanline. Returns false if the next scanline
  // is not available. This can happen when the reader cannot provide enough
  // image rows, or when all of the scanlines have been read.
  //
  virtual bool ReadNextScanline(void** out_scanline_bytes);

  // Reset the resizer to its initial state. Always returns true.
  virtual bool Reset();

  // Returns number of bytes that required to store a scanline.
  virtual size_t GetBytesPerScanline() {
    return static_cast<size_t>(elements_per_row_);
  }

  // Returns true if there are more scanlines to read. Returns false if the
  // object has not been initialized or all of the scanlines have been read.
  virtual bool HasMoreScanLines() {
    return (row_ < height_);
  }

  // Returns the height of the image.
  virtual size_t GetImageHeight() {
    return static_cast<size_t>(height_);
  }

  // Returns the width of the image.
  virtual size_t GetImageWidth() {
    return static_cast<size_t>(width_);
  }

  // Returns the pixel format of the image.
  virtual PixelFormat GetPixelFormat() {
    return reader_->GetPixelFormat();
  }

 private:
  // Compute the output size and the resizing ratios.
  void ComputeResizedSizeRatio(int input_width,
                               int input_height,
                               int output_width,
                               int output_height,
                               int* width,
                               int* height,
                               float* ratio_x,
                               float* ratio_y);

 private:
  ScanlineReaderInterface* reader_;
  // Horizontal resizer.
  scoped_ptr<ResizeRow> resizer_x_;
  // Vertical resizer.
  scoped_ptr<ResizeCol> resizer_y_;

  scoped_array<uint8> output_;
  int width_;
  int height_;
  int elements_per_row_;
  int row_;

  // Buffer for storing the intermediate results.
  scoped_array<uint8> buffer_;
  int bytes_per_buffer_row_;
  int row_buffer_;

  DISALLOW_COPY_AND_ASSIGN(ScanlineResizer);
};

}  // namespace image_compression

}  // namespace pagespeed

#endif  // THIRD_PARTY_PAGESPEED_SRC_PAGESPEED_IMAGE_COMPRESSION_IMAGE_RESIZER_H_
