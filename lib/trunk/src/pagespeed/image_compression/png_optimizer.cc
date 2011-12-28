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

#include <string>

#include "base/logging.h"

extern "C" {
#ifdef USE_SYSTEM_ZLIB
#include "zlib.h"  // NOLINT
#else
#include "third_party/zlib/zlib.h"
#endif

#include "third_party/optipng/src/opngreduc.h"
}

using pagespeed::image_compression::PngCompressParams;

namespace {

struct PngInput {
  const std::string* data_;
  int offset_;
};

// we use these four combinations because different images seem to benefit from
// different parameters and this combination of 4 seems to work best for a large
// set of PNGs from the web.
const PngCompressParams kPngCompressionParams[] = {
  PngCompressParams(PNG_ALL_FILTERS, Z_DEFAULT_STRATEGY),
  PngCompressParams(PNG_ALL_FILTERS, Z_FILTERED),
  PngCompressParams(PNG_FILTER_NONE, Z_DEFAULT_STRATEGY),
  PngCompressParams(PNG_FILTER_NONE, Z_FILTERED)
};

const size_t kParamCount = arraysize(kPngCompressionParams);

void ReadPngFromStream(png_structp read_ptr,
                       png_bytep data,
                       png_size_t length) {
  PngInput* input = reinterpret_cast<PngInput*>(png_get_io_ptr(read_ptr));
  size_t copied = input->data_->copy(reinterpret_cast<char*>(data), length,
                                     input->offset_);
  input->offset_ += copied;
  if (copied < length) {
    LOG(INFO) << "ReadPngFromStream: Unexpected EOF.";

    // We weren't able to satisfy the read, so abort.
#if PNG_LIBPNG_VER >= 10400
    png_longjmp(read_ptr, 1);
#else
    longjmp(read_ptr->jmpbuf, 1);
#endif
  }
}

void WritePngToString(png_structp write_ptr,
                      png_bytep data,
                      png_size_t length) {
  std::string& buffer =
      *reinterpret_cast<std::string*>(png_get_io_ptr(write_ptr));
  buffer.append(reinterpret_cast<char*>(data), length);
}

void PngErrorFn(png_structp png_ptr, png_const_charp msg) {
  LOG(INFO) << "libpng error: " << msg;

  // Invoking the error function indicates a terminal failure, which
  // means we must longjmp to abort the libpng invocation.
#if PNG_LIBPNG_VER >= 10400
  png_longjmp(png_ptr, 1);
#else
  longjmp(png_ptr->jmpbuf, 1);
#endif
}

void PngWarningFn(png_structp png_ptr, png_const_charp msg) {
  LOG(INFO) << "libpng warning: " << msg;
}

// no-op
void PngFlush(png_structp write_ptr) {}

// Helper that reads an unsigned 32-bit integer from a stream of
// big-endian bytes.
inline uint32 ReadUint32FromBigEndianBytes(const unsigned char* read_head) {
  return ((uint32)(*read_head) << 24) +
      ((uint32)(*(read_head + 1)) << 16) +
      ((uint32)(*(read_head + 2)) << 8) +
      (uint32)(*(read_head + 3));
}

}  // namespace

namespace pagespeed {

namespace image_compression {

PngCompressParams::PngCompressParams(int level, int strategy)
    : filter_level(level), compression_strategy(strategy) {
}

ScopedPngStruct::ScopedPngStruct(Type type)
    : png_ptr_(NULL), info_ptr_(NULL), type_(type) {
  switch (type) {
    case READ:
      png_ptr_ = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                       NULL, NULL, NULL);
      break;
    case WRITE:
      png_ptr_ = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                        NULL, NULL, NULL);
      break;
    default:
      LOG(DFATAL) << "Invalid Type " << type_;
      break;
  }
  if (png_ptr_ != NULL) {
    info_ptr_ = png_create_info_struct(png_ptr_);
  }

  png_set_error_fn(png_ptr_, NULL, &PngErrorFn, &PngWarningFn);
}

void ScopedPngStruct::reset() {
  switch (type_) {
    case READ:
      png_destroy_read_struct(&png_ptr_, &info_ptr_, NULL);
      png_ptr_ = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                        NULL, NULL, NULL);
      break;
    case WRITE:
      png_destroy_write_struct(&png_ptr_, &info_ptr_);
      png_ptr_ = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                         NULL, NULL, NULL);
      break;
    default:
      LOG(DFATAL) << "Invalid Type " << type_;
      break;
  }
  if (png_ptr_ != NULL) {
    info_ptr_ = png_create_info_struct(png_ptr_);
  }

  png_set_error_fn(png_ptr_, NULL, &PngErrorFn, &PngWarningFn);
}

ScopedPngStruct::~ScopedPngStruct() {
  switch (type_) {
    case READ:
      png_destroy_read_struct(&png_ptr_, &info_ptr_, NULL);
      break;
    case WRITE:
      png_destroy_write_struct(&png_ptr_, &info_ptr_);
      break;
    default:
      break;
  }
}

PngReaderInterface::PngReaderInterface() {
}

PngReaderInterface::~PngReaderInterface() {
}

PngOptimizer::PngOptimizer()
    : read_(ScopedPngStruct::READ),
      write_(ScopedPngStruct::WRITE),
      best_compression_(false) {
}

PngOptimizer::~PngOptimizer() {
}

bool PngOptimizer::CreateOptimizedPng(PngReaderInterface& reader,
                                      const std::string& in,
                                      std::string* out) {
  if (!read_.valid() || !write_.valid()) {
    LOG(DFATAL) << "Invalid ScopedPngStruct r: "
                << read_.valid() << ", w: " << write_.valid();
    return false;
  }

  out->clear();

  // Configure error handlers.
  if (setjmp(png_jmpbuf(read_.png_ptr()))) {
    return false;
  }

  if (setjmp(png_jmpbuf(write_.png_ptr()))) {
    return false;
  }

  if (!reader.ReadPng(in, read_.png_ptr(), read_.info_ptr(),
                      PNG_TRANSFORM_IDENTITY)) {
    return false;
  }

  if (!opng_validate_image(read_.png_ptr(), read_.info_ptr())) {
    return false;
  }

  // Copy the image data from the read structures to the write structures.
  CopyReadToWrite();

  // Perform all possible lossless image reductions
  // (e.g. RGB->palette, etc).
  opng_reduce_image(write_.png_ptr(), write_.info_ptr(), OPNG_REDUCE_ALL);

  if (best_compression_) {
    return CreateBestOptimizedPngForParams(kPngCompressionParams, kParamCount,
                                           out);
  } else {
    PngCompressParams params(PNG_FILTER_NONE, Z_DEFAULT_STRATEGY);
    return CreateOptimizedPngWithParams(&write_, params, out);
  }
}

bool PngOptimizer::CreateBestOptimizedPngForParams(
    const PngCompressParams* param_list, size_t param_list_size,
    std::string* out) {
  bool success = false;
  for (size_t idx = 0; idx < param_list_size; ++idx) {
    ScopedPngStruct write(ScopedPngStruct::WRITE);
    std::string temp_output;
    // libpng doesn't allow for reuse of the write structs, so we must copy on
    // each iteration of the loop.
    CopyPngStructs(&write_, &write);
    if (CreateOptimizedPngWithParams(&write, param_list[idx], &temp_output)) {
      // If this gives better compression update the output.
      if (out->empty() || out->size() > temp_output.size()) {
        out->swap(temp_output);
      }
      success |= true;
    }
  }
  return success;
}

bool PngOptimizer::CreateOptimizedPngWithParams(ScopedPngStruct* write,
    const PngCompressParams& params,
    std::string *out) {
  int compression_level =
      best_compression_ ? Z_BEST_COMPRESSION : Z_DEFAULT_COMPRESSION;
  png_set_compression_level(write->png_ptr(), compression_level);
  png_set_compression_mem_level(write->png_ptr(), 8);
  png_set_compression_strategy(write->png_ptr(), params.compression_strategy);
  png_set_filter(write->png_ptr(), PNG_FILTER_TYPE_BASE, params.filter_level);
  png_set_compression_window_bits(write->png_ptr(), 15);
  if (!WritePng(write, out)) {
    return false;
  }
  return true;
}

bool PngOptimizer::OptimizePng(PngReaderInterface& reader,
                               const std::string& in,
                               std::string* out) {
  PngOptimizer o;
  return o.CreateOptimizedPng(reader, in, out);
}

bool PngOptimizer::OptimizePngBestCompression(PngReaderInterface& reader,
                                              const std::string& in,
                                              std::string* out) {
  PngOptimizer o;
  o.EnableBestCompression();
  return o.CreateOptimizedPng(reader, in, out);
}

PngReader::PngReader() {
}

PngReader::~PngReader() {
}

bool PngReader::ReadPng(const std::string& body,
                        png_structp png_ptr,
                        png_infop info_ptr,
                        int transforms) {
  // Wrap the resource's response body in a structure that keeps a
  // pointer to the body and a read offset, and pass a pointer to this
  // object as the user data to be received by the PNG read function.
  PngInput input;
  input.data_ = &body;
  input.offset_ = 0;
  png_set_read_fn(png_ptr, &input, &ReadPngFromStream);
  png_read_png(png_ptr, info_ptr, transforms , NULL);
  return true;
}

bool PngReader::GetAttributes(const std::string& body,
                              int* out_width,
                              int* out_height,
                              int* out_bit_depth,
                              int* out_color_type) {
  // We need to read the PNG signature plus the IDAT chunk.
  //
  // Signature is 8 bytes, documentation:
  //  http://www.libpng.org/pub/png/spec/1.2/png-1.2-pdg.html#PNG-file-signature
  //
  // Chunk layout is 4 bytes chunk len + 4 bytes chunk name + chunk +
  // 4 bytes chunk CRC, documentation:
  //  http://www.libpng.org/pub/png/spec/1.2/png-1.2-pdg.html#Chunk-layout
  //
  // IDAT chunk is 13 bytes (see code for details), documentation:
  //  http://www.libpng.org/pub/png/spec/1.2/png-1.2-pdg.html#C.IHDR

  const size_t kPngSigBytesSize = 8;
  const size_t kChunkLenSize = 4;
  const size_t kChunkNameSize = 4;
  const size_t kIHDRChunkSize = 13;
  const size_t kChunkCRCSize = 4;

  const size_t kPngMinHeaderSize =
      kPngSigBytesSize +
      kChunkLenSize +
      kChunkNameSize +
      kIHDRChunkSize +
      kChunkCRCSize;

  if (body.size() < kPngMinHeaderSize) {
    // Not enough bytes for us to read, so abort early.
    return false;
  }

  const unsigned char* read_head =
      reinterpret_cast<const unsigned char*>(body.data());

  // Validate the PNG signature.
  if (png_sig_cmp(
          const_cast<unsigned char*>(read_head), 0, kPngSigBytesSize) != 0) {
    return false;
  }
  read_head += kPngSigBytesSize;

  // The first 4 bytes of the chunk contains the chunk length.
  const uint32 first_chunk_len = ReadUint32FromBigEndianBytes(read_head);
  if (first_chunk_len != kIHDRChunkSize) {
    return false;
  }
  read_head += kChunkLenSize;

  if (strncmp("IHDR", reinterpret_cast<const char*>(read_head), 4) != 0) {
    return false;
  }

  // Compute the CRC for the chunk (using zlib's CRC computer since
  // it's already available to us).
  uint32 computed_crc = crc32(0L, Z_NULL, 0);
  computed_crc =
      crc32(computed_crc, read_head, kChunkNameSize + kIHDRChunkSize);
  read_head += kChunkNameSize;

  // Extract the expected CRC, after the end of the IHDR data.
  uint32 expected_crc =
      ReadUint32FromBigEndianBytes(read_head + kIHDRChunkSize);
  if (expected_crc != computed_crc) {
    // CRC mismatch. Invalid chunk. Abort.
    return false;
  }

  // Now read the IHDR chunk contents. Its layout is:
  // width: 4 bytes
  // height: 4 bytes
  // bit_depth: 1 byte
  // color_type: 1 byte
  // other data: 3 bytes
  *out_width = ReadUint32FromBigEndianBytes(read_head);
  *out_height = ReadUint32FromBigEndianBytes(read_head + 4);
  *out_bit_depth = read_head[8];
  *out_color_type = read_head[9];
  return true;
}

bool PngOptimizer::WritePng(ScopedPngStruct* write, std::string* buffer) {
  png_set_write_fn(write->png_ptr(), buffer, &WritePngToString, &PngFlush);
  png_write_png(
      write->png_ptr(), write->info_ptr(), PNG_TRANSFORM_IDENTITY, NULL);

  return true;
}

void PngOptimizer::CopyReadToWrite() {
  CopyPngStructs(&read_, &write_);
}

void PngOptimizer::CopyPngStructs(ScopedPngStruct* from, ScopedPngStruct* to) {
  png_uint_32 width, height;
  int bit_depth, color_type, interlace_type, compression_type, filter_type;
  png_get_IHDR(from->png_ptr(),
               from->info_ptr(),
               &width,
               &height,
               &bit_depth,
               &color_type,
               &interlace_type,
               &compression_type,
               &filter_type);

  png_set_IHDR(to->png_ptr(),
               to->info_ptr(),
               width,
               height,
               bit_depth,
               color_type,
               interlace_type,
               compression_type,
               filter_type);

  // NOTE: if libpng's free_me capability is not enabled, sharing
  // rowbytes between the read and write structs will lead to a
  // double-free. Thus we test for the PNG_FREE_ME_SUPPORTED define
  // here.
#ifndef PNG_FREE_ME_SUPPORTED
#error PNG_FREE_ME_SUPPORTED is required or double-frees may happen.
#endif
  png_bytepp row_pointers = png_get_rows(from->png_ptr(), from->info_ptr());
  png_set_rows(to->png_ptr(), to->info_ptr(), row_pointers);

  png_colorp palette;
  int num_palette;
  if (png_get_PLTE(
          from->png_ptr(), from->info_ptr(), &palette, &num_palette) != 0) {
    png_set_PLTE(to->png_ptr(),
                 to->info_ptr(),
                 palette,
                 num_palette);
  }

  // Transparency is not considered metadata, although tRNS is
  // ancillary.
  png_bytep trans;
  int num_trans;
  png_color_16p trans_values;
  if (png_get_tRNS(from->png_ptr(),
                   from->info_ptr(),
                   &trans,
                   &num_trans,
                   &trans_values) != 0) {
    png_set_tRNS(to->png_ptr(),
                 to->info_ptr(),
                 trans,
                 num_trans,
                 trans_values);
  }

  double gamma;
  if (png_get_gAMA(from->png_ptr(), from->info_ptr(), &gamma) != 0) {
    png_set_gAMA(to->png_ptr(), to->info_ptr(), gamma);
  }

  // Do not copy bkgd, hist or sbit sections, since they are not
  // supported in most browsers.
}

bool PngReader::IsAlphaChannelOpaque(png_structp png_ptr, png_infop info_ptr) {
  png_uint_32 height;
  png_uint_32 width;
  int bit_depth;
  int color_type;

  png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
               NULL, NULL, NULL);

  if ((color_type & PNG_COLOR_MASK_ALPHA) == 0) {
    // Image doesn't have alpha.
    LOG(DFATAL) << "You shouldn't call this functions for image that doesn't"
                << "have alpha channel";
    return false;
  }

  int channels = png_get_channels(png_ptr, info_ptr);

  if (color_type == PNG_COLOR_TYPE_RGB_ALPHA) {
    if (channels != 4) {
      LOG(DFATAL) << "Encountered unexpected number of channels for RGBA"
                  << " image: " << channels;
      return false;
    }
  } else if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
    if (channels != 2) {
      LOG(DFATAL) << "Encountered unexpected number of channels for "
                  << "Gray + Alpha image: " << channels;
      return false;
    }
  } else {
    LOG(DFATAL) << "Encountered alpha image of unknown type :" << color_type;
    return false;
  }

  // We currently detect aplha only for 8/16 bit Gray/TrueColor with Alpha
  // channel. Only 8 or 16 bit depths are supports for these modes.
  if (bit_depth % 8 != 0) {
    LOG(DFATAL) << "Received unexpected bit_depth: " << bit_depth;
    return false;
  }

  int bytes_per_channel = bit_depth / 8;
  int bytes_per_pixel = channels * bytes_per_channel;
  png_bytepp row_pointers = png_get_rows(png_ptr, info_ptr);

  // Alpha channel is always the last channel.
  png_uint_32 alpha_byte_offset = (channels - 1) * bytes_per_channel;
  for (png_uint_32 row = 0; row < height; ++row) {
    unsigned char* row_bytes =
        static_cast<unsigned char*>(*(row_pointers + row));
    for (png_uint_32 pixel = 0; pixel < width * bytes_per_pixel;
         pixel += bytes_per_pixel) {
      for (int alpha_byte = 0; alpha_byte < bytes_per_channel;
           ++alpha_byte) {
        if ((row_bytes[pixel + alpha_byte_offset + alpha_byte] & 0xff) !=
            0xff) {
          return false;
        }
      }
    }
  }

  return true;
}

PngScanlineReader::PngScanlineReader()
    : read_(ScopedPngStruct::READ),
      current_scanline_(0),
      transform_(PNG_TRANSFORM_IDENTITY) {
}

jmp_buf* PngScanlineReader::GetJmpBuf() {
  jmp_buf& buf = png_jmpbuf(read_.png_ptr());
  return &buf;
}

void PngScanlineReader::Reset() {
  read_.reset();
  current_scanline_ = 0;
  transform_ = PNG_TRANSFORM_IDENTITY;
}

bool PngScanlineReader::InitializeRead(PngReaderInterface& reader,
                                       const std::string& in) {
  if (!read_.valid()) {
    LOG(DFATAL) << "Invalid ScopedPngStruct r: " << read_.valid();
    return false;
  }

  if(!reader.ReadPng(in, read_.png_ptr(), read_.info_ptr(), transform_)) {
    return false;
  }

  int color_type = png_get_color_type(read_.png_ptr(), read_.info_ptr());
  if (((color_type & PNG_COLOR_MASK_ALPHA) != 0) &&
      reader.IsAlphaChannelOpaque(read_.png_ptr(), read_.info_ptr())) {
    // Clear the read pointers.
    read_.reset();
    return reader.ReadPng(in, read_.png_ptr(), read_.info_ptr(),
                          transform_ | PNG_TRANSFORM_STRIP_ALPHA);
  }

  return true;
}

PngScanlineReader::~PngScanlineReader() {
}

size_t PngScanlineReader::GetBytesPerScanline() {
  return png_get_rowbytes(read_.png_ptr(), read_.info_ptr());
}

bool PngScanlineReader::HasMoreScanLines() {
  size_t height = png_get_image_height(read_.png_ptr(), read_.info_ptr());
  return current_scanline_ < height;
}

bool PngScanlineReader::ReadNextScanline(void** out_scanline_bytes) {
  if (!HasMoreScanLines()) {
    LOG(DFATAL) << "Read past last scanline.";
    return false;
  }

  png_bytepp row_pointers = png_get_rows(read_.png_ptr(), read_.info_ptr());
  *out_scanline_bytes = static_cast<void*>(*(row_pointers + current_scanline_));
  current_scanline_++;
  return true;
}

void PngScanlineReader::set_transform(int transform) {
  transform_ = transform;
}

size_t PngScanlineReader::GetImageHeight() {
  return png_get_image_height(read_.png_ptr(), read_.info_ptr());
}

size_t PngScanlineReader::GetImageWidth() {
  return png_get_image_width(read_.png_ptr(), read_.info_ptr());
}

int PngScanlineReader::GetColorType() {
  return png_get_color_type(read_.png_ptr(), read_.info_ptr());
}

PixelFormat PngScanlineReader::GetPixelFormat() {
  int bit_depth = png_get_bit_depth(read_.png_ptr(), read_.info_ptr());
  int color_type = png_get_color_type(read_.png_ptr(), read_.info_ptr());
  if (bit_depth == 8 && color_type == 0) {
    return GRAY_8;
  } else if (bit_depth == 8 && color_type == 2) {
    return RGB_888;
  }

  return UNSUPPORTED;
}

}  // namespace image_compression

}  // namespace pagespeed
