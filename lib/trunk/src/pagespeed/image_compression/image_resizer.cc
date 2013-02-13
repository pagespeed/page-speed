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

#include "pagespeed/image_compression/image_resizer.h"

#include <math.h>

#include "base/logging.h"
#include "pagespeed/image_compression/scanline_utils.h"

namespace pagespeed {

namespace {

// Table for storing the resizing coefficients.
struct ResizeTableEntry {
  int first_index_;
  float first_weight_;
  int last_index_;
  float last_weight_;

  ResizeTableEntry() :
    first_index_(0),
    first_weight_(0.0),
    last_index_(0),
    last_weight_(0.0) {
  }
};

// Round to the nearest integer.
inline float Round(float val) {
  return floor(val + 0.5);
}

// Check if the value is very close to the specific integer.
// This function will be used to assist IsApproximatelyZero()
// and IsApproximatelyInteger(), which will be used to optimize
// interpolation coefficients for the "area" method.
//
// The "area" method basically divides the input image into grids.
// Each grid corresponds to an output pixel and the average value
// of the input pixels within the grid determines the value for the
// output pixel. When the grid does not align with the border of
// input pixels, some input pixels will be involved to compute
// multiple (2) output pixels. When the difference between the grid
// and the border of input pixel is small, we can ignore the difference.
// Therefore we can save computation because one input pixel will only
// be used to compute one output pixel. The numerical results shall
// not have a noticable difference because we quantize the ouput to intgers
// of 0...255.
//
inline bool IsCloseToIntegerHelper(float val, float int_val) {
  // Reference: http://c-faq.com/fp/fpequal.html
  const float kThreshold = 1.0E-12 * int_val;
  bool is_integer = (val >= int_val - kThreshold) &&
      (val <= int_val + kThreshold);
  return is_integer;
}

inline bool IsApproximatelyZero(float val) {
  return IsCloseToIntegerHelper(val, 0.0);
}

inline bool IsApproximatelyInteger(float val) {
  return IsCloseToIntegerHelper(val, Round(val));
}

// Compute the interpolation coefficents for the "area" method.
// Reference for the "area" resizing method:
// http://opencv.willowgarage.com/documentation/cpp/
//     geometric_image_transformations.html
//
// The inputs, in_size and out_size, are 1-D sizes specified in pixels.
//
ResizeTableEntry* CreateTableForAreaMethod(int in_size,
                                           int out_size,
                                           float ratio) {
  if (in_size <= 0 || out_size <= 0 || ratio <= 0) {
    LOG(ERROR) << "The inputs must be positive values.";
    return NULL;
  }
  ResizeTableEntry* table = new ResizeTableEntry[out_size];
  if (table == NULL) {
    LOG(ERROR) << "Failed to allocate memory.";
    return NULL;
  }

  float end_pos = 0;
  for (int i = 0; i < out_size; ++i) {
    float start_pos = end_pos;
    float start_pos_floor = floor(start_pos);
    table[i].first_index_ = static_cast<int>(start_pos_floor);
    table[i].first_weight_ = 1.0 + start_pos_floor - start_pos;

    end_pos += ratio;
    if (IsApproximatelyInteger(end_pos)) {
      end_pos = Round(end_pos);
      table[i].last_index_ = static_cast<int>(end_pos) - 1;
    } else {
      table[i].last_index_ = static_cast<int>(end_pos);
    }
    if (table[i].last_index_ >= in_size) {
      table[i].last_index_ = in_size - 1;
    }

    if (table[i].first_index_ < table[i].last_index_) {
      table[i].last_weight_ = end_pos - table[i].last_index_;
    } else {
      table[i].last_weight_ = ratio - table[i].first_weight_;
    }
  }
  return table;
}

}  // namespace

namespace image_compression {

// Base class for the horizontal resizer.
class ResizeRow {
 public:
  virtual ~ResizeRow() {}
  virtual void Resize(const void* in_data_ptr, void* out_data_ptr) = 0;
  virtual bool Initialize(int in_size, int out_size, float ratio) = 0;
};

// Base class for the vertical resizer.
class ResizeCol {
 public:
  virtual ~ResizeCol() {}

  virtual void Resize(const void* in_data_ptr, void* out_data_ptr) = 0;

  virtual bool Initialize(int in_size,
                          int out_size,
                          float ratio_x,
                          float ratio_y,
                          int elements_per_output_row) = 0;

  virtual void SetOutputRow(int row) = 0;
  virtual int LastIndex() const = 0;
  virtual bool NeedToSaveLastRow() const = 0;
};

// Base class for the horizontal resizer using the "area" method.
class ResizeRowArea : public ResizeRow {
 public:
  virtual void Resize(const void* in_data_ptr, void* out_data_ptr) = 0;
  virtual bool Initialize(int in_size, int out_size, float ratio);

 protected:
  virtual int GetNumChannels() const = 0;

 protected:
  int pixels_per_row_;
  scoped_array<ResizeTableEntry> table_;
};

bool ResizeRowArea::Initialize(int in_size, int out_size, float ratio) {
  table_.reset(CreateTableForAreaMethod(in_size, out_size, ratio));
  if (table_ != NULL) {
    // Modify the indice so they are based on bytes in stead of pixels.
    const int num_channels = GetNumChannels();
    for (int i = 0; i < out_size; ++i) {
      table_[i].first_index_ *= num_channels;
      table_[i].last_index_ *= num_channels;
    }
    pixels_per_row_ = out_size;
    return true;
  }
  return false;
}

// Horizontal resizer for the Gray_8 format using the "area" method.
template<class OutputType>
class ResizeRowAreaGray : public ResizeRowArea {
 public:
  virtual void Resize(const void* in_data_ptr, void* out_data_ptr);

 protected:
  virtual int GetNumChannels() const {
    return 1;
  }
};

template<class OutputType>
void ResizeRowAreaGray<OutputType>::Resize(const void* in_data_ptr,
                                           void* out_data_ptr) {
  const uint8_t* in_data = reinterpret_cast<const uint8_t*>(in_data_ptr);
  OutputType* out_data = reinterpret_cast<OutputType*>(out_data_ptr);
  int out_idx = 0;
  for (int x = 0; x < pixels_per_row_; ++x) {
    int in_idx = table_[x].first_index_;
    OutputType weight = static_cast<OutputType>(table_[x].first_weight_);
    out_data[out_idx] = in_data[in_idx] * weight;
    ++in_idx;

    while (in_idx < table_[x].last_index_) {
      out_data[out_idx] += in_data[in_idx];
      ++in_idx;
    }

    weight = static_cast<OutputType>(table_[x].last_weight_);
    out_data[out_idx] += in_data[in_idx] * weight;
    ++out_idx;
  }
}

// Horizontal resizer for the RGB_888 format using the "area" method.
template<class OutputType>
class ResizeRowAreaRGB : public ResizeRowArea {
 public:
  virtual void Resize(const void* in_data_ptr, void* out_data_ptr);

 protected:
  virtual int GetNumChannels() const {
    return 3;
  }
};

template<class OutputType>
void ResizeRowAreaRGB<OutputType>::Resize(const void* in_data_ptr,
                                          void* out_data_ptr) {
  const uint8_t* in_data = reinterpret_cast<const uint8_t*>(in_data_ptr);
  OutputType* out_data = reinterpret_cast<OutputType*>(out_data_ptr);

  int out_idx = 0;
  for (int x = 0; x < pixels_per_row_; ++x) {
    int in_idx = table_[x].first_index_;
    OutputType weight = static_cast<OutputType>(table_[x].first_weight_);
    out_data[out_idx]   = in_data[in_idx]   * weight;
    out_data[out_idx+1] = in_data[in_idx+1] * weight;
    out_data[out_idx+2] = in_data[in_idx+2] * weight;
    in_idx += 3;

    while (in_idx < table_[x].last_index_) {
      out_data[out_idx]   += in_data[in_idx];
      out_data[out_idx+1] += in_data[in_idx+1];
      out_data[out_idx+2] += in_data[in_idx+2];
      in_idx += 3;
    }

    weight = static_cast<OutputType>(table_[x].last_weight_);
    out_data[out_idx]   += in_data[in_idx]   * weight;
    out_data[out_idx+1] += in_data[in_idx+1] * weight;
    out_data[out_idx+2] += in_data[in_idx+2] * weight;
    out_idx += 3;
  }
}

// Horizontal resizer for the RGBA_8888 format using the "area" method.
template<class OutputType>
class ResizeRowAreaRGBA : public ResizeRowArea {
 public:
  virtual void Resize(const void* in_data_ptr, void* out_data_ptr);

 protected:
  virtual int GetNumChannels() const {
    return 4;
  }
};

template<class OutputType>
void ResizeRowAreaRGBA<OutputType>::Resize(const void* in_data_ptr,
                                           void* out_data_ptr) {
  const uint8_t* in_data = reinterpret_cast<const uint8_t*>(in_data_ptr);
  OutputType* out_data = reinterpret_cast<OutputType*>(out_data_ptr);

  int out_idx = 0;
  for (int x = 0; x < pixels_per_row_; ++x) {
    int in_idx = table_[x].first_index_;
    OutputType weight = static_cast<OutputType>(table_[x].first_weight_);
    out_data[out_idx]   = in_data[in_idx]   * weight;
    out_data[out_idx+1] = in_data[in_idx+1] * weight;
    out_data[out_idx+2] = in_data[in_idx+2] * weight;
    out_data[out_idx+3] = in_data[in_idx+3] * weight;
    in_idx += 4;

    while (in_idx < table_[x].last_index_) {
      out_data[out_idx]   += in_data[in_idx];
      out_data[out_idx+1] += in_data[in_idx+1];
      out_data[out_idx+2] += in_data[in_idx+2];
      out_data[out_idx+3] += in_data[in_idx+3];
      in_idx += 4;
    }

    weight = static_cast<OutputType>(table_[x].last_weight_);
    out_data[out_idx]   += in_data[in_idx]   * weight;
    out_data[out_idx+1] += in_data[in_idx+1] * weight;
    out_data[out_idx+2] += in_data[in_idx+2] * weight;
    out_data[out_idx+3] += in_data[in_idx+3] * weight;
    out_idx += 4;
  }
}

// Vertical resizer for all pixel formats using the "area" method.
template<class InputType, class BufferType>
class ResizeColArea : public ResizeCol {
 public:
  virtual void Resize(const void* in_data_ptr, void* out_data_ptr);

  virtual bool Initialize(int in_size,
                          int out_size,
                          float ratio_x,
                          float ratio_y,
                          int elements_per_output_row);

  void SetOutputRow(int row) {
    row_ = row;
  }

  int LastIndex() const {
    return table_[row_].last_index_;
  }

  bool NeedToSaveLastRow() const {
    return !IsApproximatelyInteger(table_[row_].last_weight_);
  }

 private:
  void AlignTableIndex(int out_size) {
    for (int i = 0; i < out_size; ++i) {
      table_[i].last_index_ -= table_[i].first_index_;
      table_[i].first_index_ = 0;
    }
  }

 private:
  scoped_array<ResizeTableEntry> table_;
  scoped_array<BufferType> buffer_;
  int elements_per_row_;
  int row_;
  BufferType grid_area_;
  BufferType half_grid_area_;
};

template<class InputType, class BufferType>
bool ResizeColArea<InputType, BufferType>::Initialize(
    int in_size,
    int out_size,
    float ratio_x,
    float ratio_y,
    int elements_per_output_row) {
  table_.reset(CreateTableForAreaMethod(in_size, out_size, ratio_y));
  if (table_ == NULL) {
    return false;
  }
  AlignTableIndex(out_size);

  buffer_.reset(new BufferType[elements_per_output_row]);
  if (buffer_ == NULL) {
    return false;
  }

  grid_area_ = static_cast<BufferType>(ratio_x)
      * static_cast<BufferType>(ratio_y);
  half_grid_area_ = grid_area_ / 2;
  elements_per_row_ = elements_per_output_row;
  return true;
}

// Resize the image vertically and output a row. Loop unrolling is used
// for speed performance.
//
template<class InputType, class BufferType>
void ResizeColArea<InputType, BufferType>::Resize(const void* in_data_ptr,
                                                    void* out_data_ptr) {
  const InputType* in_data = reinterpret_cast<const InputType*>(in_data_ptr);
  uint8_t* out_data = reinterpret_cast<uint8_t*>(out_data_ptr);

  const ResizeTableEntry& table = table_[row_];
  BufferType weight = static_cast<BufferType>(table.first_weight_);
  int in_idx = 0;
  int out_idx = 0;
  const int elements_per_row_4 =
      (elements_per_row_ >= 4 ? elements_per_row_ - 4 : 0);

  for (out_idx = 0; out_idx < elements_per_row_4; in_idx+=4, out_idx+=4) {
    buffer_[out_idx]   = weight * static_cast<BufferType>(in_data[in_idx]);
    buffer_[out_idx+1] = weight * static_cast<BufferType>(in_data[in_idx+1]);
    buffer_[out_idx+2] = weight * static_cast<BufferType>(in_data[in_idx+2]);
    buffer_[out_idx+3] = weight * static_cast<BufferType>(in_data[in_idx+3]);
  }
  for (; out_idx < elements_per_row_; ++in_idx, ++out_idx) {
    buffer_[out_idx] = weight * static_cast<BufferType>(in_data[in_idx]);
  }

  for (int y = 1; y < table.last_index_; ++y) {
    for (out_idx = 0; out_idx < elements_per_row_4; in_idx+=4, out_idx+=4) {
      buffer_[out_idx]   += static_cast<BufferType>(in_data[in_idx]);
      buffer_[out_idx+1] += static_cast<BufferType>(in_data[in_idx+1]);
      buffer_[out_idx+2] += static_cast<BufferType>(in_data[in_idx+2]);
      buffer_[out_idx+3] += static_cast<BufferType>(in_data[in_idx+3]);
    }
    for (; out_idx < elements_per_row_; ++in_idx, ++out_idx) {
      buffer_[out_idx] += static_cast<BufferType>(in_data[in_idx]);
    }
  }

  weight = static_cast<BufferType>(table.last_weight_);
  for (out_idx = 0; out_idx < elements_per_row_4; in_idx+=4, out_idx+=4) {
    out_data[out_idx] = static_cast<uint8_t>((
        static_cast<BufferType>(in_data[in_idx]) * weight +
        buffer_[out_idx] + half_grid_area_) / grid_area_);

    out_data[out_idx+1] = static_cast<uint8_t>((
        static_cast<BufferType>(in_data[in_idx+1]) * weight +
        buffer_[out_idx+1] + half_grid_area_) / grid_area_);

    out_data[out_idx+2] = static_cast<uint8_t>((
        static_cast<BufferType>(in_data[in_idx+2]) * weight +
        buffer_[out_idx+2] + half_grid_area_) / grid_area_);

    out_data[out_idx+3] = static_cast<uint8_t>((
        static_cast<BufferType>(in_data[in_idx+3]) * weight +
        buffer_[out_idx+3] + half_grid_area_) / grid_area_);
  }
  for (; out_idx < elements_per_row_; ++in_idx, ++out_idx) {
    out_data[out_idx] = static_cast<uint8_t>((
        static_cast<BufferType>(in_data[in_idx]) * weight +
        buffer_[out_idx] + half_grid_area_) / grid_area_);
  }
}

// Intantiate the resizers. It is based on the pixel format as well as the
// resizing ratios.
template<class ResizeXType, class ResizeYType>
bool InstantiateResizers(pagespeed::image_compression::PixelFormat pixel_format,
                         scoped_ptr<ResizeRow>* resizer_x,
                         scoped_ptr<ResizeCol>* resizer_y) {
  bool is_ok = true;
  switch (pixel_format) {
    case GRAY_8:
      resizer_x->reset(new ResizeRowAreaGray<ResizeXType>());
      break;
    case RGB_888:
      resizer_x->reset(new ResizeRowAreaRGB<ResizeXType>());
      break;
    case RGBA_8888:
      resizer_x->reset(new ResizeRowAreaRGBA<ResizeXType>());
      break;
    default:
      LOG(ERROR) << "Invalid pixel format.";
      is_ok = false;
  }
  resizer_y->reset(new ResizeColArea<ResizeXType, ResizeYType>());
  is_ok = (is_ok && resizer_x != NULL && resizer_y != NULL);
  return is_ok;
}

ScanlineResizer::ScanlineResizer()
  : reader_(NULL),
    width_(0),
    height_(0),
    elements_per_row_(0),
    row_(0),
    bytes_per_buffer_row_(0),
    row_buffer_(0) {
}

ScanlineResizer::~ScanlineResizer() {
}

// Reset the scanline reader to its initial state.
bool ScanlineResizer::Reset() {
  reader_ = NULL;
  width_ = 0;
  height_ = 0;
  elements_per_row_ = 0;
  row_ = 0;
  bytes_per_buffer_row_ = 0;
  row_buffer_ = 0;
  return true;
}

// Reads the next available scanline.
bool ScanlineResizer::ReadNextScanline(void** out_scanline_bytes) {
  if (!HasMoreScanLines()) {
    return false;
  }

  // Fetch scanlines from the reader until we have enough input rows for
  // computing an output row.
  resizer_y_->SetOutputRow(row_);
  while (row_buffer_ <= resizer_y_->LastIndex()) {
    if (!reader_->HasMoreScanLines()) {
      return false;
    }
    void* in_scanline_bytes = NULL;
    if (!reader_->ReadNextScanline(&in_scanline_bytes)) {
      return false;
    }

    // Resize the input scanline horizontally and put the results in buffer_.
    resizer_x_->Resize(in_scanline_bytes,
                       &buffer_[row_buffer_ * bytes_per_buffer_row_]);
    ++row_buffer_;
  }

  // Now we have enough scanlines, we can resize the image vertically and put
  // the results to the output.
  resizer_y_->Resize(buffer_.get(), output_.get());

  // If the weight for the last input row is not an integer, this row will be
  // used to compute the next output row, so it is copied to the top of the
  // buffer.
  if (resizer_y_->NeedToSaveLastRow()) {
    memcpy(&buffer_[0],
           &buffer_[resizer_y_->LastIndex() * bytes_per_buffer_row_],
           bytes_per_buffer_row_);
    row_buffer_ = 1;
  } else {
    row_buffer_ = 0;
  }
  ++row_;
  *out_scanline_bytes = output_.get();
  return true;
}

// Compute the output size and resizing ratios.
void ScanlineResizer::ComputeResizedSizeRatio(int input_width,
                                              int input_height,
                                              int output_width,
                                              int output_height,
                                              int* width,
                                              int* height,
                                              float* ratio_x,
                                              float* ratio_y) {
  float original_width = static_cast<float>(input_width);
  float original_height = static_cast<float>(input_height);
  float resized_width = static_cast<float>(output_width);
  float resized_height = static_cast<float>(output_height);

  if (resized_width > 0.0 && resized_height > 0.0) {
    *ratio_x = original_width / resized_width;
    *ratio_y = original_height / resized_height;
  } else if (resized_width > 0.0) {
    *ratio_x = original_width / resized_width;
    *ratio_y = *ratio_x;
    resized_height = Round(original_height / *ratio_y);
  } else if (resized_height > 0.0) {
    *ratio_y = original_height / resized_height;
    *ratio_x = *ratio_y;
    resized_width = Round(original_width / *ratio_x);
  } else {
    // This should never happen because the inputs have been checked in
    // Initialize().
    LOG(ERROR) << "Either the output width or height or both must be positive.";
  }

  *width = static_cast<int>(resized_width);
  *height = static_cast<int>(resized_height);
}

// Initialize the resizer. For computational efficiency, we try to use
// integer for internal compuation and buffer if it is possible. In particular,
// - If both ratio_x and ratio_y are integers, use integer for all computation;
// - If ratio_x is an integer but ratio_y is not, use integer for the
//   horizontal resizer and floating point for the vertical resizer;
// - Otherwise, use floating point for all compuation.
//
bool ScanlineResizer::Initialize(ScanlineReaderInterface* reader,
                                 size_t output_width,
                                 size_t output_height) {
  if (reader == NULL ||
      reader->GetImageWidth() == 0 ||
      reader->GetImageHeight() == 0) {
    LOG(ERROR) << "The input image cannot be empty.";
    return false;
  }

  if (output_width == 0 && output_height == 0) {
    LOG(ERROR) << "Either the width or height, or both, must be positive.";
    return false;
  }

  const int input_width = static_cast<int>(reader->GetImageWidth());
  const int input_height = static_cast<int>(reader->GetImageHeight());
  int resized_width, resized_height;
  float ratio_x, ratio_y;

  ComputeResizedSizeRatio(input_width,
                          input_height,
                          static_cast<int>(output_width),
                          static_cast<int>(output_height),
                          &resized_width,
                          &resized_height,
                          &ratio_x,
                          &ratio_y);

  if (ratio_x < 1 || ratio_y < 1) {
    // We are using the "area" method for resizing image. This method is good
    // for shrinking, but not enlarging.
    LOG(ERROR) << "Enlarging image is not supported";
    return false;
  }

  const bool is_ratio_x_integer = IsApproximatelyInteger(ratio_x);
  const bool is_ratio_y_integer = IsApproximatelyInteger(ratio_y);

  reader_ = reader;
  height_ = resized_height;
  width_ = resized_width;
  row_ = 0;
  row_buffer_ = 0;
  const PixelFormat pixel_format = reader->GetPixelFormat();
  elements_per_row_ = resized_width *
      pagespeed::image_compression::GetNumChannelsFromPixelFormat(pixel_format);

  // The height of buffer is 1 more than the ratio. The additional "1" is
  // because partial rows of the input may be used to compute an ouput row.
  // For example, at the ratio of 1.4 we may need to use three input rows,
  // with weights such as 0.2, 1, and 0.2.
  const int buffer_height = static_cast<int>(ceil(ratio_y)) + 1;

  if (is_ratio_x_integer && is_ratio_y_integer) {
    // Use uint32_t for buffer and intermediate computation.
    InstantiateResizers<uint32_t, uint32_t>(pixel_format, &resizer_x_,
                                            &resizer_y_);
    bytes_per_buffer_row_ = elements_per_row_ * sizeof(uint32_t);
  } else if (is_ratio_x_integer) {
    // Use uint32_t for horizontal resizer and float for vertical resizer.
    InstantiateResizers<uint32_t, float>(pixel_format, &resizer_x_,
                                         &resizer_y_);
    bytes_per_buffer_row_ = elements_per_row_ * sizeof(uint32_t);
  } else {
    // Use float for buffer and intermediate computation.
    InstantiateResizers<float, float>(pixel_format, &resizer_x_, &resizer_y_);
    bytes_per_buffer_row_ = elements_per_row_ * sizeof(float);
  }
  buffer_.reset(new uint8_t[buffer_height * bytes_per_buffer_row_]);
  output_.reset(new uint8_t[elements_per_row_]);

  if (buffer_ == NULL || output_ == NULL) {
    return false;
  }

  if (!resizer_x_->Initialize(input_width, resized_width, ratio_x)) {
    return false;
  }
  if (!resizer_y_->Initialize(input_height, resized_height,
                              ratio_x, ratio_y, elements_per_row_)) {
    return false;
  }

  return true;
}

}  // namespace image_compression

}  // namespace pagespeed
