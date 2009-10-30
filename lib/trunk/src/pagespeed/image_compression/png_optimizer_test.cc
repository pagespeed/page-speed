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
#include "pagespeed/image_compression/png_optimizer.h"
#include "third_party/libpng/png.h"
#include "third_party/readpng/cpp/readpng.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace {

using pagespeed::image_compression::PngOptimizer;

// The PNG_TEST_DIR_PATH macro is set by the gyp target that builds this file.
const std::string kPngTestDir = PNG_TEST_DIR_PATH;

void ReadFileToString(const char* file_name, std::string *dest) {
  const std::string path = kPngTestDir + file_name;
  std::ifstream file_stream;
  file_stream.open(path.c_str(), std::ifstream::in | std::ifstream::binary);
  dest->assign(std::istreambuf_iterator<char>(file_stream),
               std::istreambuf_iterator<char>());
  file_stream.close();
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
  // background chunks match.
#if defined(PNG_bKGD_SUPPORTED) || defined(PNG_READ_BACKGROUND_SUPPORTED)
  EXPECT_EQ(orig_desc.bgcolor_retval, opt_desc.bgcolor_retval)
      << "readpng_get_bgcolor mismatch for " << identifier;
  if (orig_desc.bgcolor_retval == 0 && opt_desc.bgcolor_retval == 0) {
    EXPECT_EQ(orig_desc.bg_red, opt_desc.bg_red)
        << "red mismatch for " << identifier;
    EXPECT_EQ(orig_desc.bg_green, opt_desc.bg_green)
        << "green mismatch for " << identifier;
    EXPECT_EQ(orig_desc.bg_blue, opt_desc.bg_blue)
        << "blue mismatch for " << identifier;
  }
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
  { "basi0g01.png", 217, 217 },
  { "basi0g02.png", 154, 154 },
  { "basi0g04.png", 247, 247 },
  { "basi0g08.png", 254, 1018 },
  { "basi0g16.png", 299, 1542 },
  { "basi2c08.png", 315, 1827 },
  { "basi2c16.png", 595, 3058 },
  { "basi3p01.png", 132, 132 },
  { "basi3p02.png", 193, 178 },
  { "basi3p04.png", 327, 312 },
  { "basi3p08.png", 1527, 1556 },
  { "basi4a08.png", 214, 1442 },
  { "basi4a16.png", 2855, 2733 },
  { "basi6a08.png", 361, 1443 },
  { "basi6a16.png", 4180, 5128 },
  { "basn0g01.png", 164, 164 },
  { "basn0g02.png", 104, 104 },
  { "basn0g04.png", 145, 145 },
  { "basn0g08.png", 138, 1166 },
  { "basn0g16.png", 167, 1515 },
  { "basn2c08.png", 145, 2079 },
  { "basn2c16.png", 302, 3028 },
  { "basn3p01.png", 112, 112 },
  { "basn3p02.png", 146, 131 },
  { "basn3p04.png", 216, 201 },
  { "basn3p08.png", 1286, 1286 },
  { "basn4a08.png", 126, 1423 },
  { "basn4a16.png", 2206, 2302 },
  { "basn6a08.png", 184, 1432 },
  { "basn6a16.png", 3435, 5231 },
  { "bgai4a08.png", 214, 1442 },
  { "bgai4a16.png", 2855, 2733 },
  { "bgan6a08.png", 184, 1432 },
  { "bgan6a16.png", 3435, 5231 },
  { "bgbn4a08.png", 140, 1423 },
  { "bggn4a16.png", 2220, 2302 },
  { "bgwn6a08.png", 202, 1432 },
  { "bgyn6a16.png", 3453, 5231 },
  { "ccwn2c08.png", 1514, 1723 },
  { "ccwn3p08.png", 1554, 1516 },
  { "cdfn2c08.png", 404, 532 },
  { "cdhn2c08.png", 344, 491 },
  { "cdsn2c08.png", 232, 258 },
  { "cdun2c08.png", 724, 952 },
  { "ch1n3p04.png", 258, 201 },
  { "ch2n3p08.png", 1810, 1286 },
  { "cm0n0g04.png", 292, 273 },
  { "cm7n0g04.png", 292, 273 },
  { "cm9n0g04.png", 292, 273 },
  { "cs3n2c16.png", 214, 226 },
  { "cs3n3p08.png", 259, 244 },
  { "cs5n2c08.png", 186, 256 },
  { "cs5n3p08.png", 271, 256 },
  { "cs8n2c08.png", 149, 256 },
  { "cs8n3p08.png", 256, 256 },
  { "ct0n0g04.png", 273, 273 },
  { "ct1n0g04.png", 792, 273 },
  { "ctzn0g04.png", 753, 273 },
  { "f00n0g08.png", 319, 323 },
  { "f00n2c08.png", 2475, 2457 },
  { "f01n0g08.png", 321, 282 },
  { "f01n2c08.png", 1180, 2534 },
  { "f02n0g08.png", 355, 297 },
  { "f02n2c08.png", 1729, 2494 },
  { "f03n0g08.png", 389, 294 },
  { "f03n2c08.png", 1291, 2492 },
  { "f04n0g08.png", 269, 291 },
  { "f04n2c08.png", 985, 2533 },
  { "g03n0g16.png", 345, 291 },
  { "g03n2c08.png", 370, 492 },
  { "g03n3p04.png", 214, 214 },
  { "g04n0g16.png", 363, 295 },
  { "g04n2c08.png", 377, 497 },
  { "g04n3p04.png", 219, 219 },
  { "g05n0g16.png", 339, 291 },
  { "g05n2c08.png", 350, 491 },
  { "g05n3p04.png", 206, 206 },
  { "g07n0g16.png", 321, 290 },
  { "g07n2c08.png", 340, 492 },
  { "g07n3p04.png", 207, 207 },
  { "g10n0g16.png", 262, 292 },
  { "g10n2c08.png", 285, 498 },
  { "g10n3p04.png", 214, 214 },
  { "g25n0g16.png", 383, 294 },
  { "g25n2c08.png", 405, 470 },
  { "g25n3p04.png", 215, 215 },
  { "oi1n0g16.png", 167, 1515 },
  { "oi1n2c16.png", 302, 3028 },
  { "oi2n0g16.png", 179, 1515 },
  { "oi2n2c16.png", 314, 3028 },
  { "oi4n0g16.png", 203, 1515 },
  { "oi4n2c16.png", 338, 3028 },
  { "oi9n0g16.png", 1283, 1515 },
  { "oi9n2c16.png", 3038, 3028 },
  { "pp0n2c16.png", 962, 3028 },
  { "pp0n6a08.png", 818, 2975 },
  { "ps1n0g08.png", 1477, 1166 },
  { "ps1n2c16.png", 1641, 3028 },
  { "ps2n0g08.png", 2341, 1166 },
  { "ps2n2c16.png", 2505, 3028 },
  { "s01i3p01.png", 113, 98 },
  { "s01n3p01.png", 113, 98 },
  { "s02i3p01.png", 114, 99 },
  { "s02n3p01.png", 115, 100 },
  { "s03i3p01.png", 118, 103 },
  { "s03n3p01.png", 120, 105 },
  { "s04i3p01.png", 126, 111 },
  { "s04n3p01.png", 121, 106 },
  { "s05i3p02.png", 134, 119 },
  { "s05n3p02.png", 129, 114 },
  { "s06i3p02.png", 143, 128 },
  { "s06n3p02.png", 131, 116 },
  { "s07i3p02.png", 149, 134 },
  { "s07n3p02.png", 138, 123 },
  { "s08i3p02.png", 149, 134 },
  { "s08n3p02.png", 139, 124 },
  { "s09i3p02.png", 147, 132 },
  { "s09n3p02.png", 143, 128 },
  { "s32i3p04.png", 355, 338 },
  { "s32n3p04.png", 263, 247 },
  { "s33i3p04.png", 385, 370 },
  { "s33n3p04.png", 329, 314 },
  { "s34i3p04.png", 349, 332 },
  { "s34n3p04.png", 248, 233 },
  { "s35i3p04.png", 399, 383 },
  { "s35n3p04.png", 338, 320 },
  { "s36i3p04.png", 356, 339 },
  { "s36n3p04.png", 258, 242 },
  { "s37i3p04.png", 393, 377 },
  { "s37n3p04.png", 336, 318 },
  { "s38i3p04.png", 357, 339 },
  { "s38n3p04.png", 245, 231 },
  { "s39i3p04.png", 420, 401 },
  { "s39n3p04.png", 352, 346 },
  { "s40i3p04.png", 357, 340 },
  { "s40n3p04.png", 256, 241 },
  { "tbbn1g04.png", 419, 401 },
  { "tbbn2c16.png", 1994, 1114 },
  { "tbbn3p08.png", 1128, 1114 },
  { "tbgn2c16.png", 1994, 1114 },
  { "tbgn3p08.png", 1128, 1114 },
  { "tbrn2c08.png", 1347, 1114 },
  { "tbwn1g16.png", 1146, 595 },
  { "tbwn3p08.png", 1131, 1114 },
  { "tbyn3p08.png", 1131, 1114 },
  { "tp0n1g08.png", 689, 581 },
  { "tp0n2c08.png", 1311, 1111 },
  { "tp0n3p08.png", 1120, 1110 },
  { "tp1n3p08.png", 1115, 1114 },
  { "z00n2c08.png", 3172, 1945 },
  { "z03n2c08.png", 232, 1945 },
  { "z06n2c08.png", 224, 1945 },
  { "z09n2c08.png", 224, 1945 },
};

const char *kInvalidFiles[] = {
  "nosuchfile.png",
  "emptyfile.png",
  "x00n0g01.png",
  "xcrn0g04.png",
  "xlfn0g04.png",
};

const size_t kValidImageCount = arraysize(kValidImages);
const size_t kInvalidFileCount = arraysize(kInvalidFiles);

TEST(PngOptimizerTest, ValidPngs) {
  for (int i = 0; i < kValidImageCount; i++) {
    std::string in, out;
    ReadFileToString(kValidImages[i].filename, &in);
    ASSERT_TRUE(PngOptimizer::OptimizePng(in, &out))
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
  for (int i = 0; i < kInvalidFileCount; i++) {
    std::string in, out;
    ReadFileToString(kInvalidFiles[i], &in);
    ASSERT_FALSE(PngOptimizer::OptimizePng(in, &out));
  }
}

// Make sure that after we fail, we're still able to successfully
// compress valid images.
TEST(PngOptimizerTest, SuccessAfterFailure) {
  for (int i = 0; i < kInvalidFileCount; i++) {
    {
      std::string in, out;
      ReadFileToString(kInvalidFiles[i], &in);
      ASSERT_FALSE(PngOptimizer::OptimizePng(in, &out));
    }

    {
      std::string in, out;
      ReadFileToString(kValidImages[i].filename, &in);
      ASSERT_TRUE(PngOptimizer::OptimizePng(in, &out));
    }
  }
}

}  // namespace
