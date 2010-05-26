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

#include <fstream>
#include <sstream>
#include <string>

#include "base/basictypes.h"
#ifdef PAGESPEED_PNG_OPTIMIZER_GIF_READER
#include "pagespeed/image_compression/gif_reader.h"
#endif
#include "pagespeed/image_compression/png_optimizer.h"
#include "third_party/libpng/png.h"
#include "third_party/readpng/cpp/readpng.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace {

#ifdef PAGESPEED_PNG_OPTIMIZER_GIF_READER
using pagespeed::image_compression::GifReader;
#endif
using pagespeed::image_compression::PngOptimizer;
using pagespeed::image_compression::PngReader;
using pagespeed::image_compression::ScopedPngStruct;

// The *_TEST_DIR_PATH macros are set by the gyp target that builds this file.
const std::string kGifTestDir = IMAGE_TEST_DIR_PATH "gif/";
const std::string kPngSuiteTestDir = IMAGE_TEST_DIR_PATH "pngsuite/";
const std::string kPngTestDir = IMAGE_TEST_DIR_PATH "png/";

void ReadFileToString(const std::string& dir,
                      const char* file_name,
                      const char* ext,
                      std::string* dest) {
  const std::string path = dir + file_name + '.' + ext;
  std::ifstream file_stream;
  file_stream.open(path.c_str(), std::ifstream::in | std::ifstream::binary);
  dest->assign(std::istreambuf_iterator<char>(file_stream),
               std::istreambuf_iterator<char>());
  file_stream.close();
}

void ReadPngSuiteFileToString(const char* file_name, std::string* dest) {
  ReadFileToString(kPngSuiteTestDir, file_name, "png", dest);
}

// Structure that holds metadata and actual pixel data for a decoded
// PNG.
struct ReadPngDescriptor {
  unsigned char* img_bytes;  // The actual pixel data.
  unsigned long width;
  unsigned long height;
  int channels;              // 3 for RGB, 4 for RGB+alpha
  unsigned long row_bytes;   // number of bytes in a row
  unsigned char bg_red, bg_green, bg_blue;
  int bgcolor_retval;
};

// Decode the PNG and store the decoded data and metadata in the
// ReadPngDescriptor struct.
void PopulateDescriptor(const std::string& img,
                        ReadPngDescriptor* desc,
                        const char* identifier) {
  readpng::ReadPNG reader;
  std::istringstream stream(img, std::istringstream::binary);
  ASSERT_EQ(0, reader.readpng_init(stream, &desc->width, &desc->height))
      << "Failed to init for img " << identifier;
#if defined(PNG_bKGD_SUPPORTED) || defined(PNG_READ_BACKGROUND_SUPPORTED)
  desc->bgcolor_retval = reader.readpng_get_bgcolor(&desc->bg_red,
                                                    &desc->bg_green,
                                                    &desc->bg_blue);
#endif
  desc->img_bytes = reader.readpng_get_image(&desc->channels, &desc->row_bytes);
  reader.readpng_cleanup(0);
}

void AssertPngEq(
    const std::string& orig, const std::string& opt, const char* identifier) {
  // Gather data and metadata for the original and optimized PNGs.
  ReadPngDescriptor orig_desc;
  PopulateDescriptor(orig, &orig_desc, identifier);
  ReadPngDescriptor opt_desc;
  PopulateDescriptor(opt, &opt_desc, identifier);

  // Verify that the dimensions match.
  EXPECT_EQ(orig_desc.width, opt_desc.width)
      << "width mismatch for " << identifier;
  EXPECT_EQ(orig_desc.height, opt_desc.height)
      << "height mismatch for " << identifier;

  // If PNG background chunks are supported, verify that the
  // background chunks are not present in the optimized image.
#if defined(PNG_bKGD_SUPPORTED) || defined(PNG_READ_BACKGROUND_SUPPORTED)
  EXPECT_EQ(1, opt_desc.bgcolor_retval) << "Unexpected: bgcolor";
#endif

  // Verify that the number of channels matches (should be 3 for RGB
  // or 4 for RGB+alpha)
  EXPECT_EQ(orig_desc.channels, opt_desc.channels)
      << "channel mismatch for " << identifier;

  // Verify that the number of bytes in a row matches.
  EXPECT_EQ(orig_desc.row_bytes, opt_desc.row_bytes)
      << "row_bytes mismatch for " << identifier;

  // Verify that the actual image data matches.
  if (orig_desc.row_bytes == opt_desc.row_bytes &&
      orig_desc.height == opt_desc.height) {
    const unsigned long img_bytes_size = orig_desc.row_bytes * orig_desc.height;
    EXPECT_EQ(0,
              memcmp(orig_desc.img_bytes, opt_desc.img_bytes, img_bytes_size))
        << "image data mismatch for " << identifier;
  }

  free(orig_desc.img_bytes);
  free(opt_desc.img_bytes);
}

struct ImageCompressionInfo {
  const char* filename;
  int original_size;
  int compressed_size;
};

ImageCompressionInfo kValidImages[] = {
  { "basi0g01", 217, 217 },
  { "basi0g02", 154, 154 },
  { "basi0g04", 247, 247 },
  { "basi0g08", 254, 1018 },
  { "basi0g16", 299, 1542 },
  { "basi2c08", 315, 1827 },
  { "basi2c16", 595, 3058 },
  { "basi3p01", 132, 132 },
  { "basi3p02", 193, 178 },
  { "basi3p04", 327, 312 },
  { "basi3p08", 1527, 1556 },
  { "basi4a08", 214, 1442 },
  { "basi4a16", 2855, 2733 },
  { "basi6a08", 361, 1443 },
  { "basi6a16", 4180, 5128 },
  { "basn0g01", 164, 164 },
  { "basn0g02", 104, 104 },
  { "basn0g04", 145, 145 },
  { "basn0g08", 138, 1166 },
  { "basn0g16", 167, 1515 },
  { "basn2c08", 145, 2079 },
  { "basn2c16", 302, 3028 },
  { "basn3p01", 112, 112 },
  { "basn3p02", 146, 131 },
  { "basn3p04", 216, 201 },
  { "basn3p08", 1286, 1286 },
  { "basn4a08", 126, 1423 },
  { "basn4a16", 2206, 2302 },
  { "basn6a08", 184, 1432 },
  { "basn6a16", 3435, 5231 },
  { "bgai4a08", 214, 1442 },
  { "bgai4a16", 2855, 2733 },
  { "bgan6a08", 184, 1432 },
  { "bgan6a16", 3435, 5231 },
  { "bgbn4a08", 140, 1423 },
  { "bggn4a16", 2220, 2302 },
  { "bgwn6a08", 202, 1432 },
  { "bgyn6a16", 3453, 5231 },
  { "ccwn2c08", 1514, 1723 },
  { "ccwn3p08", 1554, 1516 },
  { "cdfn2c08", 404, 532 },
  { "cdhn2c08", 344, 491 },
  { "cdsn2c08", 232, 258 },
  { "cdun2c08", 724, 952 },
  { "ch1n3p04", 258, 201 },
  { "ch2n3p08", 1810, 1286 },
  { "cm0n0g04", 292, 273 },
  { "cm7n0g04", 292, 273 },
  { "cm9n0g04", 292, 273 },
  { "cs3n2c16", 214, 226 },
  { "cs3n3p08", 259, 244 },
  { "cs5n2c08", 186, 256 },
  { "cs5n3p08", 271, 256 },
  { "cs8n2c08", 149, 256 },
  { "cs8n3p08", 256, 256 },
  { "ct0n0g04", 273, 273 },
  { "ct1n0g04", 792, 273 },
  { "ctzn0g04", 753, 273 },
  { "f00n0g08", 319, 323 },
  { "f00n2c08", 2475, 2457 },
  { "f01n0g08", 321, 282 },
  { "f01n2c08", 1180, 2534 },
  { "f02n0g08", 355, 297 },
  { "f02n2c08", 1729, 2494 },
  { "f03n0g08", 389, 294 },
  { "f03n2c08", 1291, 2492 },
  { "f04n0g08", 269, 291 },
  { "f04n2c08", 985, 2533 },
  { "g03n0g16", 345, 291 },
  { "g03n2c08", 370, 492 },
  { "g03n3p04", 214, 214 },
  { "g04n0g16", 363, 295 },
  { "g04n2c08", 377, 497 },
  { "g04n3p04", 219, 219 },
  { "g05n0g16", 339, 291 },
  { "g05n2c08", 350, 491 },
  { "g05n3p04", 206, 206 },
  { "g07n0g16", 321, 290 },
  { "g07n2c08", 340, 492 },
  { "g07n3p04", 207, 207 },
  { "g10n0g16", 262, 292 },
  { "g10n2c08", 285, 498 },
  { "g10n3p04", 214, 214 },
  { "g25n0g16", 383, 294 },
  { "g25n2c08", 405, 470 },
  { "g25n3p04", 215, 215 },
  { "oi1n0g16", 167, 1515 },
  { "oi1n2c16", 302, 3028 },
  { "oi2n0g16", 179, 1515 },
  { "oi2n2c16", 314, 3028 },
  { "oi4n0g16", 203, 1515 },
  { "oi4n2c16", 338, 3028 },
  { "oi9n0g16", 1283, 1515 },
  { "oi9n2c16", 3038, 3028 },
  { "pp0n2c16", 962, 3028 },
  { "pp0n6a08", 818, 2975 },
  { "ps1n0g08", 1477, 1166 },
  { "ps1n2c16", 1641, 3028 },
  { "ps2n0g08", 2341, 1166 },
  { "ps2n2c16", 2505, 3028 },
  { "s01i3p01", 113, 98 },
  { "s01n3p01", 113, 98 },
  { "s02i3p01", 114, 99 },
  { "s02n3p01", 115, 100 },
  { "s03i3p01", 118, 103 },
  { "s03n3p01", 120, 105 },
  { "s04i3p01", 126, 111 },
  { "s04n3p01", 121, 106 },
  { "s05i3p02", 134, 119 },
  { "s05n3p02", 129, 114 },
  { "s06i3p02", 143, 128 },
  { "s06n3p02", 131, 116 },
  { "s07i3p02", 149, 134 },
  { "s07n3p02", 138, 123 },
  { "s08i3p02", 149, 134 },
  { "s08n3p02", 139, 124 },
  { "s09i3p02", 147, 132 },
  { "s09n3p02", 143, 128 },
  { "s32i3p04", 355, 338 },
  { "s32n3p04", 263, 247 },
  { "s33i3p04", 385, 370 },
  { "s33n3p04", 329, 314 },
  { "s34i3p04", 349, 332 },
  { "s34n3p04", 248, 233 },
  { "s35i3p04", 399, 383 },
  { "s35n3p04", 338, 320 },
  { "s36i3p04", 356, 339 },
  { "s36n3p04", 258, 242 },
  { "s37i3p04", 393, 377 },
  { "s37n3p04", 336, 318 },
  { "s38i3p04", 357, 339 },
  { "s38n3p04", 245, 231 },
  { "s39i3p04", 420, 401 },
  { "s39n3p04", 352, 346 },
  { "s40i3p04", 357, 340 },
  { "s40n3p04", 256, 241 },
  { "tbbn1g04", 419, 401 },
  { "tbbn2c16", 1994, 1114 },
  { "tbbn3p08", 1128, 1114 },
  { "tbgn2c16", 1994, 1114 },
  { "tbgn3p08", 1128, 1114 },
  { "tbrn2c08", 1347, 1114 },
  { "tbwn1g16", 1146, 595 },
  { "tbwn3p08", 1131, 1114 },
  { "tbyn3p08", 1131, 1114 },
  { "tp0n1g08", 689, 581 },
  { "tp0n2c08", 1311, 1111 },
  { "tp0n3p08", 1120, 1110 },
  { "tp1n3p08", 1115, 1114 },
  { "z00n2c08", 3172, 1945 },
  { "z03n2c08", 232, 1945 },
  { "z06n2c08", 224, 1945 },
  { "z09n2c08", 224, 1945 },
};

ImageCompressionInfo kValidGifImages[] = {
  { "basi0g01", 153, 166 },
  { "basi0g02", 185, 112 },
  { "basi0g04", 344, 186 },
  { "basi0g08", 1736, 1150 },
  { "basi3p01", 138, 96 },
  { "basi3p02", 186, 115 },
  { "basi3p04", 344, 185 },
  { "basi3p08", 1737, 1270 },
  { "basn0g01", 153, 166 },
  { "basn0g02", 185, 112 },
  { "basn0g04", 344, 186 },
  { "basn0g08", 1736, 1150 },
  { "basn3p01", 138, 96 },
  { "basn3p02", 186, 115 },
  { "basn3p04", 344, 185 },
  { "basn3p08", 1737, 1270 },
};

const char* kInvalidFiles[] = {
  "nosuchfile",
  "emptyfile",
  "x00n0g01",
  "xcrn0g04",
  "xlfn0g04",
};

const size_t kValidImageCount = arraysize(kValidImages);
const size_t kValidGifImageCount = arraysize(kValidGifImages);
const size_t kInvalidFileCount = arraysize(kInvalidFiles);

TEST(PngOptimizerTest, ValidPngs) {
  PngReader reader;
  for (int i = 0; i < kValidImageCount; i++) {
    std::string in, out;
    ReadPngSuiteFileToString(kValidImages[i].filename, &in);
    ASSERT_TRUE(PngOptimizer::OptimizePng(reader, in, &out))
        << kValidImages[i].filename;
    EXPECT_EQ(kValidImages[i].original_size, in.size())
        << kValidImages[i].filename;
    EXPECT_EQ(kValidImages[i].compressed_size, out.size())
        << kValidImages[i].filename;

    // Make sure the pixels in the original match the pixels in the
    // optimized version.
    AssertPngEq(in, out, kValidImages[i].filename);
  }
}

TEST(PngOptimizerTest, InvalidPngs) {
  PngReader reader;
  for (int i = 0; i < kInvalidFileCount; i++) {
    std::string in, out;
    ReadPngSuiteFileToString(kInvalidFiles[i], &in);
    ASSERT_FALSE(PngOptimizer::OptimizePng(reader, in, &out));
  }
}

TEST(PngOptimizerTest, FixPngOutOfBoundReadCrash) {
  PngReader reader;
  std::string in, out;
  ReadFileToString(kPngTestDir, "read_from_stream_crash", "png", &in);
  ASSERT_EQ(193, in.length());
  ASSERT_FALSE(PngOptimizer::OptimizePng(reader, in, &out));
}

#ifdef PAGESPEED_PNG_OPTIMIZER_GIF_READER
TEST(PngOptimizerTest, ValidGifs) {
  GifReader reader;
  for (int i = 0; i < kValidGifImageCount; i++) {
    std::string in, out, ref;
    ReadFileToString(
        kPngSuiteTestDir + "gif/", kValidGifImages[i].filename, "gif", &in);
    ReadPngSuiteFileToString(kValidGifImages[i].filename, &ref);
    ASSERT_TRUE(PngOptimizer::OptimizePng(reader, in, &out))
        << kValidGifImages[i].filename;
    EXPECT_EQ(kValidGifImages[i].original_size, in.size())
        << kValidGifImages[i].filename;
    EXPECT_EQ(kValidGifImages[i].compressed_size, out.size())
        << kValidGifImages[i].filename;

    // Make sure the pixels in the original match the pixels in the
    // optimized version.
    AssertPngEq(ref, out, kValidGifImages[i].filename);
  }
}

TEST(PngOptimizerTest, AnimatedGif) {
  GifReader reader;
  std::string in, out;
  ReadFileToString(kGifTestDir, "animated", "gif", &in);
  ASSERT_NE(0, in.length());
  ASSERT_FALSE(PngOptimizer::OptimizePng(reader, in, &out));
}

TEST(PngOptimizerTest, InvalidGifs) {
  // Verify that we fail gracefully when trying to parse PNGs using
  // the GIF reader.
  GifReader reader;
  for (int i = 0; i < kValidImageCount; i++) {
    std::string in, out;
    ReadPngSuiteFileToString(kValidImages[i].filename, &in);
    ASSERT_FALSE(PngOptimizer::OptimizePng(reader, in, &out));
  }

  // Also verify we fail gracefully for the invalid PNG images.
  for (int i = 0; i < kInvalidFileCount; i++) {
    std::string in, out;
    ReadPngSuiteFileToString(kInvalidFiles[i], &in);
    ASSERT_FALSE(PngOptimizer::OptimizePng(reader, in, &out));
  }
}
#endif

// Make sure that after we fail, we're still able to successfully
// compress valid images.
TEST(PngOptimizerTest, SuccessAfterFailure) {
  PngReader reader;
  for (int i = 0; i < kInvalidFileCount; i++) {
    {
      std::string in, out;
      ReadPngSuiteFileToString(kInvalidFiles[i], &in);
      ASSERT_FALSE(PngOptimizer::OptimizePng(reader, in, &out));
    }

    {
      std::string in, out;
      ReadPngSuiteFileToString(kValidImages[i].filename, &in);
      ASSERT_TRUE(PngOptimizer::OptimizePng(reader, in, &out));
    }
  }
}

TEST(PngOptimizerTest, ScopedPngStruct) {
  ScopedPngStruct read(ScopedPngStruct::READ);
  ASSERT_TRUE(read.valid());
  ASSERT_NE(static_cast<png_structp>(NULL), read.png_ptr());
  ASSERT_NE(static_cast<png_infop>(NULL), read.info_ptr());

  ScopedPngStruct write(ScopedPngStruct::WRITE);
  ASSERT_TRUE(write.valid());
  ASSERT_NE(static_cast<png_structp>(NULL), write.png_ptr());
  ASSERT_NE(static_cast<png_infop>(NULL), write.info_ptr());

#ifdef NDEBUG
  ScopedPngStruct invalid(static_cast<ScopedPngStruct::Type>(-1));
  ASSERT_FALSE(invalid.valid());
  ASSERT_EQ(static_cast<png_structp>(NULL), invalid.png_ptr());
  ASSERT_EQ(static_cast<png_infop>(NULL), invalid.info_ptr());
#else
  ASSERT_DEATH(ScopedPngStruct(static_cast<ScopedPngStruct::Type>(-1)),
               "Invalid Type");
#endif
}

}  // namespace
