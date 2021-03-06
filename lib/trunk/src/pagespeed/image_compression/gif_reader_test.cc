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

// Test that basic GifReader operations succeed or fail as expected.
// Note that read-in file contents are tested against golden RGBA
// files in png_optimizer_test.cc, not here.

// Author: Victor Chudnovsky


#include "pagespeed/image_compression/gif_reader.h"
#include "pagespeed/image_compression/png_optimizer.h"
#include "pagespeed/testing/pagespeed_test.h"
#include "third_party/libpng/png.h"

namespace {

using pagespeed::image_compression::GifReader;
using pagespeed::image_compression::PngReaderInterface;
using pagespeed::image_compression::ScopedPngStruct;

const char *kValidOpaqueGifImages[] = {
  "basi0g01",
  "basi0g02",
  "basi0g04",
  "basi0g08",
  "basi3p01",
  "basi3p02",
  "basi3p04",
  "basi3p08",
  "basn0g01",
  "basn0g02",
  "basn0g04",
  "basn0g08",
  "basn3p01",
  "basn3p02",
  "basn3p04",
  "basn3p08",
};

const char *kValidTransparentGifImages[] = {
  "tr-basi4a08",
  "tr-basn4a08"
};

const size_t kValidOpaqueGifImageCount = arraysize(kValidOpaqueGifImages);
const size_t kValidTransparentGifImageCount =
    arraysize(kValidTransparentGifImages);

// The *_TEST_DIR_PATH macros are set by the gyp target that builds this file.
const char kGifTestDir[] = IMAGE_TEST_DIR_PATH "gif/";
const char kPngSuiteTestDir[] = IMAGE_TEST_DIR_PATH "pngsuite/";
const char kPngSuiteGifTestDir[] = IMAGE_TEST_DIR_PATH "pngsuite/gif/";
const char kPngTestDir[] = IMAGE_TEST_DIR_PATH "png/";

void ReadImageToString(const std::string& dir,
                       const char* file_name,
                       const char* ext,
                       std::string* dest) {
  const std::string path = dir + file_name + '.' + ext;
  pagespeed_testing::ReadFileToString(path, dest);
}

TEST(GifReaderTest, LoadValidGifsWithoutTransforms) {
  ScopedPngStruct read(ScopedPngStruct::READ);
  scoped_ptr<PngReaderInterface> gif_reader(new GifReader);
  std::string in, out;
  for (size_t i = 0; i < kValidOpaqueGifImageCount; i++) {
    ReadImageToString(
        kPngSuiteGifTestDir, kValidOpaqueGifImages[i], "gif", &in);
    ASSERT_NE(static_cast<size_t>(0), in.length());
    ASSERT_TRUE(gif_reader->ReadPng(in, read.png_ptr(), read.info_ptr(),
                                   PNG_TRANSFORM_IDENTITY))
        << kValidOpaqueGifImages[i];
    ASSERT_TRUE(read.reset());
  }

  for (size_t i = 0; i < kValidTransparentGifImageCount; i++) {
    ReadImageToString(
        kPngSuiteGifTestDir, kValidTransparentGifImages[i], "gif", &in);
    ASSERT_NE(static_cast<size_t>(0), in.length());
    ASSERT_TRUE(gif_reader->ReadPng(in, read.png_ptr(), read.info_ptr(),
                                   PNG_TRANSFORM_IDENTITY))
        << kValidTransparentGifImages[i];
    ASSERT_TRUE(read.reset());
  }

  ReadImageToString(kGifTestDir, "transparent", "gif", &in);
  ASSERT_NE(static_cast<size_t>(0), in.length());
  ASSERT_TRUE(gif_reader->ReadPng(in, read.png_ptr(), read.info_ptr(),
                                 PNG_TRANSFORM_IDENTITY));
}

TEST(GifReaderTest, ExpandColorMapForValidGifs) {
  ScopedPngStruct read(ScopedPngStruct::READ);
  scoped_ptr<PngReaderInterface> gif_reader(new GifReader);
  std::string in, out;
  for (size_t i = 0; i < kValidOpaqueGifImageCount; i++) {
    ReadImageToString(
        kPngSuiteGifTestDir, kValidOpaqueGifImages[i], "gif", &in);
    ASSERT_NE(static_cast<size_t>(0), in.length());
    ASSERT_TRUE(gif_reader->ReadPng(in, read.png_ptr(), read.info_ptr(),
                                   PNG_TRANSFORM_EXPAND))
        << kValidOpaqueGifImages[i];
    ASSERT_TRUE(read.reset());
  }

  for (size_t i = 0; i < kValidTransparentGifImageCount; i++) {
    ReadImageToString(
        kPngSuiteGifTestDir, kValidTransparentGifImages[i], "gif", &in);
    ASSERT_NE(static_cast<size_t>(0), in.length());
    ASSERT_TRUE(gif_reader->ReadPng(in, read.png_ptr(), read.info_ptr(),
                                   PNG_TRANSFORM_EXPAND))
        << kValidTransparentGifImages[i];
    ASSERT_TRUE(read.reset());
  }

  ReadImageToString(kGifTestDir, "transparent", "gif", &in);
  ASSERT_NE(static_cast<size_t>(0), in.length());
  ASSERT_TRUE(gif_reader->ReadPng(in, read.png_ptr(), read.info_ptr(),
                                 PNG_TRANSFORM_EXPAND));
}

TEST(GifReaderTest, RequireOpaqueForValidGifs) {
  ScopedPngStruct read(ScopedPngStruct::READ);
  scoped_ptr<PngReaderInterface> gif_reader(new GifReader);
  std::string in;
  for (size_t i = 0; i < kValidOpaqueGifImageCount; i++) {
    ReadImageToString(
        kPngSuiteGifTestDir, kValidOpaqueGifImages[i], "gif", &in);
    ASSERT_NE(static_cast<size_t>(0), in.length());
    ASSERT_TRUE(gif_reader->ReadPng(in, read.png_ptr(), read.info_ptr(),
                                    PNG_TRANSFORM_IDENTITY, true))
        << kValidOpaqueGifImages[i];
    ASSERT_TRUE(read.reset());
  }

  for (size_t i = 0; i < kValidTransparentGifImageCount; i++) {
    ReadImageToString(
        kPngSuiteGifTestDir, kValidTransparentGifImages[i], "gif", &in);
    ASSERT_NE(static_cast<size_t>(0), in.length());
    ASSERT_FALSE(gif_reader->ReadPng(in, read.png_ptr(), read.info_ptr(),
                                    PNG_TRANSFORM_IDENTITY, true))
        << kValidTransparentGifImages[i];
    ASSERT_TRUE(read.reset());
  }

  ReadImageToString(kGifTestDir, "transparent", "gif", &in);
  ASSERT_NE(static_cast<size_t>(0), in.length());
  ASSERT_FALSE(gif_reader->ReadPng(in, read.png_ptr(), read.info_ptr(),
                                   PNG_TRANSFORM_IDENTITY, true));
}

TEST(GifReaderTest, ExpandColormapAndRequireOpaqueForValidGifs) {
  ScopedPngStruct read(ScopedPngStruct::READ);
  scoped_ptr<PngReaderInterface> gif_reader(new GifReader);
  std::string in;
  for (size_t i = 0; i < kValidOpaqueGifImageCount; i++) {
    ReadImageToString(
        kPngSuiteGifTestDir, kValidOpaqueGifImages[i], "gif", &in);
    ASSERT_NE(static_cast<size_t>(0), in.length());
    ASSERT_TRUE(gif_reader->ReadPng(in, read.png_ptr(), read.info_ptr(),
                                    PNG_TRANSFORM_EXPAND, true))
        << kValidOpaqueGifImages[i];
    ASSERT_TRUE(read.reset());
  }

  for (size_t i = 0; i < kValidTransparentGifImageCount; i++) {
    ReadImageToString(
        kPngSuiteGifTestDir, kValidTransparentGifImages[i], "gif", &in);
    ASSERT_NE(static_cast<size_t>(0), in.length());
    ASSERT_FALSE(gif_reader->ReadPng(in, read.png_ptr(), read.info_ptr(),
                                    PNG_TRANSFORM_EXPAND, true))
        << kValidTransparentGifImages[i];
    ASSERT_TRUE(read.reset());
  }

  ReadImageToString(kGifTestDir, "transparent", "gif", &in);
  ASSERT_NE(static_cast<size_t>(0), in.length());
  ASSERT_FALSE(gif_reader->ReadPng(in, read.png_ptr(), read.info_ptr(),
                                   PNG_TRANSFORM_EXPAND, true));
}

TEST(GifReaderTest, StripAlpha) {
  ScopedPngStruct read(ScopedPngStruct::READ);
  scoped_ptr<PngReaderInterface> gif_reader(new GifReader);
  std::string in;
  png_uint_32 height;
  png_uint_32 width;
  int bit_depth;
  int color_type;
  png_bytep trans;
  int num_trans;
  png_color_16p trans_values;

  ReadImageToString(kGifTestDir, "transparent", "gif", &in);
  ASSERT_NE(static_cast<size_t>(0), in.length());
  ASSERT_TRUE(gif_reader->ReadPng(in, read.png_ptr(), read.info_ptr(),
                                  PNG_TRANSFORM_STRIP_ALPHA, false));
  png_get_IHDR(read.png_ptr(), read.info_ptr(),
               &width, &height, &bit_depth, &color_type,
               NULL, NULL, NULL);
  ASSERT_TRUE((color_type & PNG_COLOR_MASK_ALPHA) == 0);

  ASSERT_EQ(static_cast<unsigned int>(0),
            png_get_tRNS(read.png_ptr(),
                         read.info_ptr(),
                         &trans,
                         &num_trans,
                         &trans_values));

  read.reset();

  ASSERT_TRUE(gif_reader->ReadPng(in, read.png_ptr(), read.info_ptr(),
                                  PNG_TRANSFORM_STRIP_ALPHA |
                                  PNG_TRANSFORM_EXPAND, false));
  png_get_IHDR(read.png_ptr(), read.info_ptr(),
               &width, &height, &bit_depth, &color_type,
               NULL, NULL, NULL);
  ASSERT_TRUE((color_type & PNG_COLOR_MASK_ALPHA) == 0);

  ASSERT_EQ(static_cast<unsigned int>(0),
            png_get_tRNS(read.png_ptr(),
                         read.info_ptr(),
                         &trans,
                         &num_trans,
                         &trans_values));
}

TEST(GifReaderTest, ExpandColormapOnZeroSizeCanvasAndCatchLibPngError) {
  ScopedPngStruct read(ScopedPngStruct::READ);
  scoped_ptr<PngReaderInterface> gif_reader(new GifReader);
  std::string in;
  // This is a free image from
  // <http://www.gifs.net/subcategory/40/0/20/Email>, with the canvas
  // size information manually set to zero in order to trigger a
  // libpng error.
  ReadImageToString(kGifTestDir, "zero_size_animation", "gif", &in);
  ASSERT_NE(static_cast<size_t>(0), in.length());
  ASSERT_FALSE(gif_reader->ReadPng(in, read.png_ptr(), read.info_ptr(),
                                   PNG_TRANSFORM_EXPAND, true));
}

}  // namespace
