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

#include <fstream>
#include <sstream>
#include <string>

#include "base/basictypes.h"
#include "pagespeed/image_compression/image_resizer.h"
#include "pagespeed/image_compression/jpeg_optimizer.h"
#include "pagespeed/image_compression/png_optimizer.h"
#include "pagespeed/image_compression/scanline_utils.h"
#include "pagespeed/image_compression/webp_optimizer.h"
#include "pagespeed/testing/pagespeed_test.h"
#include "third_party/libpng/png.h"

namespace {

// Pixel formats
using pagespeed::image_compression::PixelFormat;
using pagespeed::image_compression::GRAY_8;
using pagespeed::image_compression::RGB_888;
using pagespeed::image_compression::RGBA_8888;
// Readers and writers
using pagespeed::image_compression::JpegCompressionOptions;
using pagespeed::image_compression::JpegScanlineWriter;
using pagespeed::image_compression::PngScanlineReaderRaw;
using pagespeed::image_compression::ScanlineResizer;
using pagespeed::image_compression::ScanlineWriterInterface;
using pagespeed::image_compression::WebpConfiguration;
using pagespeed::image_compression::WebpScanlineWriter;
using pagespeed::string_util::StringPrintf;

// The *_TEST_DIR_PATH macros are set by the gyp target that builds this file.
const char kPngSuiteTestDir[] = IMAGE_TEST_DIR_PATH "pngsuite/";
const char kResizedTestDir[] = IMAGE_TEST_DIR_PATH "resized/";

// Three testing images: GRAY_8, RGB_888, and RGBA_8888.
// Size of these images is 32-by-32 pixels.
const char* kValidImages[] = {
  "basi0g04",
  "basi3p02",
  "basn6a16",
};

// Size of the output image [width, height]. The size of the input image
// is 32-by-32. We would like to test resizing ratioes of both integers
// and non-integers.
const size_t kOutputSize[][2] = {
  {16, 0},   // Shrink image by 2 times in both directions.
  {0, 8},    // Shrink image by 4 times in both directions.
  {3, 3},    // Shrink image by 32/3 times in both directions.
  {16, 25},  // Shrink image by [2, 32/25] times.
  {32, 5},   // Shrink image by [1, 32/5] times.
  {32, 32},  // Although the image is not shrinked, the algorithm is exercised.
};

const size_t kValidImageCount = arraysize(kValidImages);
const size_t KOutputSizeCount = arraysize(kOutputSize);

bool ReadImageToString(const char* file_name,
                       std::string* image_data) {
  const std::string path = kPngSuiteTestDir + std::string(file_name) + ".png";
  return pagespeed_testing::ReadFileToString(path, image_data);
}

// Generate file name for the resized image. The size is embedded into the
// file name. For example
// pagespeed/image_compression/testdata/resized/basi0g04_w16_h16.png
//
void NameForResizedImage(const char* file_name,
                         const char* file_ext,
                         size_t width,
                         size_t height,
                         std::string* path) {
  *path = StringPrintf("%s%s_w%ld_h%ld.%s",
                       kResizedTestDir, file_name, width, height, file_ext);
}

bool ReadGoldImageToString(const char* file_name,
                           size_t width,
                           size_t height,
                           std::string* image_data) {
  std::string path;
  NameForResizedImage(file_name, "png", width, height, &path);
  return pagespeed_testing::ReadFileToString(path, image_data);
}

// Return JPEG writer for Gray_8, and WebP writer for RGB_888 or RGBA_8888.
ScanlineWriterInterface* CreateWriter(PixelFormat pixel_format,
                                      size_t width,
                                      size_t height,
                                      std::string* image_data,
                                      std::string* file_ext) {
  if (pixel_format == GRAY_8) {
    JpegScanlineWriter* jpeg_writer = new JpegScanlineWriter();
    if (jpeg_writer != NULL) {
      JpegCompressionOptions jpeg_options;
      jpeg_options.lossy = true;
      jpeg_options.lossy_options.quality = 100;

      jmp_buf env;
      if (setjmp(env)) {
        // This code is run only when libjpeg hit an error, and called
        // longjmp(env).
        jpeg_writer->AbortWrite();
      } else {
        jpeg_writer->SetJmpBufEnv(&env);
        jpeg_writer->Init(width, height, pixel_format);
        jpeg_writer->SetJpegCompressParams(jpeg_options);
        jpeg_writer->InitializeWrite(image_data);
      }
      *file_ext = "jpg";
    }
    return reinterpret_cast<ScanlineWriterInterface*>(jpeg_writer);
  } else {
    WebpScanlineWriter* webp_writer = new WebpScanlineWriter();
    if (webp_writer != NULL) {
      WebpConfiguration webp_config;  // Use lossless by default
      webp_writer->Init(width, height, pixel_format);
      webp_writer->InitializeWrite(webp_config, image_data);
      *file_ext = "webp";
    }
    return reinterpret_cast<ScanlineWriterInterface*>(webp_writer);
  }
}

// Make sure the resized results, include image size, pixel format, and pixel
// values, match the gold data. The gold data has the image size coded in the
// file name. For example, an image resized to 16-by-16 is
// pagespeed/image_compression/testdata/resized/basi0g04_w16_h16.png
//
TEST(ScanlineResizer, Accuracy) {
  PngScanlineReaderRaw reader;
  ScanlineResizer resizer;
  PngScanlineReaderRaw gold_reader;

  for (size_t index_image = 0;
       index_image < kValidImageCount;
       ++index_image) {
    std::string input_image;
    const char* file_name = kValidImages[index_image];
    ASSERT_TRUE(ReadImageToString(file_name, &input_image));

    for (size_t index_size = 0; index_size < KOutputSizeCount; ++index_size) {
      size_t width = kOutputSize[index_size][0];
      size_t height = kOutputSize[index_size][1];
      ASSERT_TRUE(reader.Initialize(input_image.data(), input_image.length()));
      ASSERT_TRUE(resizer.Initialize(&reader, width, height));

      if (width == 0) width = height;
      if (height == 0) height = width;
      std::string gold_image;
      ASSERT_TRUE(ReadGoldImageToString(file_name, width, height, &gold_image));
      ASSERT_TRUE(gold_reader.Initialize(gold_image.data(),
                                         gold_image.length()));

      // Make sure the images sizes and the pixel formats are the same.
      ASSERT_EQ(gold_reader.GetImageWidth(), resizer.GetImageWidth());
      ASSERT_EQ(gold_reader.GetImageHeight(), resizer.GetImageHeight());
      ASSERT_EQ(gold_reader.GetPixelFormat(), resizer.GetPixelFormat());

      while (resizer.HasMoreScanLines() && gold_reader.HasMoreScanLines()) {
        uint8* resized_row = NULL;
        uint8* gold_row = NULL;
        ASSERT_TRUE(resizer.ReadNextScanline(
            reinterpret_cast<void**>(&resized_row)));
        ASSERT_TRUE(gold_reader.ReadNextScanline(
            reinterpret_cast<void**>(&gold_row)));

        for (size_t i = 0; i < resizer.GetBytesPerScanline(); ++i) {
          ASSERT_EQ(gold_row[i], resized_row[i]);
        }
      }

      // Make sure both the resizer and the reader have exhausted scanlines.
      ASSERT_FALSE(resizer.HasMoreScanLines());
      ASSERT_FALSE(gold_reader.HasMoreScanLines());
    }
  }
}

// Resize the image and write the result to a JPEG or a WebP image.
// Change WRITE_RESIZED_IMAGES to 1 to write the image to disk.
//
TEST(ScanlineResizer, ResizeAndWrite) {
  PngScanlineReaderRaw reader;
  ScanlineResizer resizer;

  for (size_t index_image = 0; index_image < kValidImageCount; ++index_image) {
    std::string input_image;
    const char* file_name = kValidImages[index_image];
    ASSERT_TRUE(ReadImageToString(file_name, &input_image));

    for (size_t index_size = 0; index_size < KOutputSizeCount; ++index_size) {
      const size_t width = kOutputSize[index_size][0];
      const size_t height = kOutputSize[index_size][1];
      ASSERT_TRUE(reader.Initialize(input_image.data(), input_image.length()));
      ASSERT_TRUE(resizer.Initialize(&reader, width, height));

      std::string output_image;
      std::string file_ext;
      scoped_ptr<ScanlineWriterInterface> writer(
         CreateWriter(resizer.GetPixelFormat(),
                      resizer.GetImageWidth(),
                      resizer.GetImageHeight(),
                      &output_image,
                      &file_ext));

      while (resizer.HasMoreScanLines()) {
        void* scanline;
        ASSERT_TRUE(resizer.ReadNextScanline(&scanline));
        ASSERT_TRUE(writer->WriteNextScanline(scanline));
      }

      ASSERT_TRUE(writer->FinalizeWrite());
    }
  }
}

TEST(ScanlineResizer, Initialization) {
  PngScanlineReaderRaw reader;
  ScanlineResizer resizer;

  std::string input_image;
  const char* file_name = kValidImages[0];
  ASSERT_TRUE(ReadImageToString(file_name, &input_image));
  ASSERT_TRUE(reader.Initialize(input_image.data(), input_image.length()));

  size_t width, height;

  // Both width and height are specified.
  width = 20;
  height = 10;
  ASSERT_TRUE(resizer.Initialize(&reader, width, height));
  EXPECT_EQ(width, resizer.GetImageWidth());
  EXPECT_EQ(height, resizer.GetImageHeight());
  EXPECT_EQ(reader.GetPixelFormat(), resizer.GetPixelFormat());

  // Only height is specified.
  width = 0;
  height = 10;
  ASSERT_TRUE(resizer.Initialize(&reader, width, height));
  EXPECT_EQ(height, resizer.GetImageWidth());
  EXPECT_EQ(height, resizer.GetImageHeight());

  // Only width is specified.
  width = 12;
  height = 0;
  ASSERT_TRUE(resizer.Initialize(&reader, width, height));
  EXPECT_EQ(width, resizer.GetImageWidth());
  EXPECT_EQ(width, resizer.GetImageHeight());

  // Enlarging image is prohibited (size of the input image is 32-by-32).
  width = 1000;
  height = 10;
  EXPECT_FALSE(resizer.Initialize(&reader, width, height));

  // Shrinking image to empty is prohibited.
  width = 0;
  height = 0;
  EXPECT_FALSE(resizer.Initialize(&reader, width, height));

  // Uninitialized reader.
  width = 20;
  height = 10;
  reader.Reset();
  EXPECT_FALSE(resizer.Initialize(&reader, width, height));
}

TEST(ScanlineResizer, ReadNextScanline) {
  ScanlineResizer resizer;
  void* scanline = NULL;
  // The resizer is not initialized, so read will return false.
  ASSERT_FALSE(resizer.ReadNextScanline(&scanline));

  PngScanlineReaderRaw reader;
  std::string input_image;
  const char* file_name = kValidImages[1];
  ASSERT_TRUE(ReadImageToString(file_name, &input_image));
  ASSERT_TRUE(reader.Initialize(input_image.data(), input_image.size()));
  ASSERT_TRUE(resizer.Initialize(&reader, 10, 1));
  ASSERT_TRUE(resizer.ReadNextScanline(&scanline));
  // The resizer has only one scanline, so any further read will return false.
  ASSERT_FALSE(resizer.ReadNextScanline(&scanline));
}

TEST(ScanlineResizer, BadReader) {
  PngScanlineReaderRaw reader;
  ScanlineResizer resizer;
  std::string input_image;
  const char* file_name = kValidImages[0];

  ASSERT_TRUE(ReadImageToString(file_name, &input_image));
  // Truncate the input image string. Only 100 bytes are passed to the reader.
  // The reader is able decode the image header, but not the pixels.
  ASSERT_TRUE(reader.Initialize(input_image.data(), 100));
  ASSERT_TRUE(resizer.Initialize(&reader, 10, 20));

  void* scanline = NULL;
  ASSERT_FALSE(resizer.ReadNextScanline(&scanline));
}

TEST(ScanlineResizer, PartialRead) {
  PngScanlineReaderRaw reader;
  ScanlineResizer resizer;
  std::string input_image;
  const char* file_name = kValidImages[0];
  void* scanline = NULL;

  ASSERT_TRUE(ReadImageToString(file_name, &input_image));

  // Read only 1 scanline, although there are 20.
  ASSERT_TRUE(reader.Initialize(input_image.data(), input_image.size()));
  ASSERT_TRUE(resizer.Initialize(&reader, 10, 20));
  EXPECT_TRUE(resizer.ReadNextScanline(&scanline));

  // Read only 2 scanlines, although there are 20.
  ASSERT_TRUE(reader.Initialize(input_image.data(), input_image.size()));
  ASSERT_TRUE(resizer.Initialize(&reader, 10, 20));
  EXPECT_TRUE(resizer.ReadNextScanline(&scanline));
  EXPECT_TRUE(resizer.ReadNextScanline(&scanline));
}

}  // namespace
