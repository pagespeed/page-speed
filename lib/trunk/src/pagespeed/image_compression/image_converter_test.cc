/**
 * Copyright 2011 Google Inc.
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

// Author: Satyanarayana Manyam

#include <fstream>
#include <sstream>
#include <string>

#include "base/logging.h"

#include "base/basictypes.h"
#include "pagespeed/image_compression/image_converter.h"
#include "pagespeed/testing/pagespeed_test.h"

namespace {

using pagespeed::image_compression::ImageConverter;
using pagespeed::image_compression::JpegLossyOptions;
using pagespeed::image_compression::PngReader;
using pagespeed::image_compression::WebpConfiguration;

// The *_TEST_DIR_PATH macro is set by the gyp target that builds this file.
const std::string kPngSuiteTestDir = IMAGE_TEST_DIR_PATH "pngsuite/";

struct ImageCompressionInfo {
  const char* filename;
  size_t original_size;
  size_t compressed_size;
  bool is_png;
  size_t webp_lossless_size;
  size_t webp_lossy_q50m0aq50_size;
};

// These images were obtained from
// http://www.libpng.org/pub/png/pngsuite.html
ImageCompressionInfo kValidImages[] = {
  { "basi0g01", 217, 208, 1, 120, 390},
  { "basi0g02", 154, 154, 1, 74, 98},
  { "basi0g04", 247, 145, 1, 104, 140},
  { "basi0g08", 254, 250, 1, 582, 156},
  { "basi0g16", 299, 285, 1, 458, 150},
  { "basi2c08", 315, 313, 1, 88, 180},
  { "basi2c16", 595, 419, 0, 134, 196},
  { "basi3p01", 132, 132, 1, 56, 300},
  { "basi3p02", 193, 178, 1, 74, 360},
  { "basi3p04", 327, 312, 1, 136, 306},
  { "basi3p08", 1527, 565, 0, 810, 314},
  { "basi4a08", 214, 209, 1, 80, 1154},
  { "basi4a16", 2855, 1980, 1, 748, 1374},
  { "basi6a08", 361, 350, 1, 132, 1262},
  { "basi6a16", 4180, 4133, 1, 652, 1310},
  { "basn0g01", 164, 164, 1, 120, 390},
  { "basn0g02", 104, 104, 1, 74, 98},
  { "basn0g04", 145, 103, 1, 104, 140},
  { "basn0g08", 138, 132, 1, 582, 156},
  { "basn0g16", 167, 152, 1, 458, 150},
  { "basn2c08", 145, 145, 1, 88, 180},
  { "basn2c16", 302, 274, 1, 134, 196},
  { "basn3p01", 112, 112, 1, 56, 300},
  { "basn3p02", 146, 131, 1, 74, 360},
  { "basn3p04", 216, 201, 1, 136, 306},
  { "basn3p08", 1286, 565, 0, 810, 314},
  { "basn4a08", 126, 121, 1, 80, 1154},
  { "basn4a16", 2206, 1185, 1, 748, 1374},
  { "basn6a08", 184, 176, 1, 132, 1262},
  { "basn6a16", 3435, 3271, 1, 652, 1310},
  { "bgai4a08", 214, 209, 1, 80, 1154},
  { "bgai4a16", 2855, 1980, 1, 748, 1374},
  { "bgan6a08", 184, 176, 1, 132, 1262},
  { "bgan6a16", 3435, 3271, 1, 652, 1310},
  { "bgbn4a08", 140, 121, 1, 80, 1154},
  { "bggn4a16", 2220, 1185, 1, 748, 1374},
  { "bgwn6a08", 202, 176, 1, 132, 1262},
  { "bgyn6a16", 3453, 3271, 1, 652, 1310},
  { "ccwn2c08", 1514, 764, 0, 1220, 394},
  { "ccwn3p08", 1554, 779, 0, 1234, 380},
  { "cdfn2c08", 404, 498, 1, 348, 246},
  { "cdhn2c08", 344, 476, 1, 312, 230},
  { "cdsn2c08", 232, 255, 1, 156, 158},
  { "cdun2c08", 724, 928, 1, 638, 486},
  { "ch1n3p04", 258, 201, 1, 136, 306},
  { "ch2n3p08", 1810, 565, 0, 810, 314},
  { "cm0n0g04", 292, 271, 1, 232, 578},
  { "cm7n0g04", 292, 271, 1, 232, 578},
  { "cm9n0g04", 292, 271, 1, 232, 578},
  { "cs3n2c16", 214, 178, 1, 116, 212},
  { "cs3n3p08", 259, 244, 1, 116, 216},
  { "cs5n2c08", 186, 226, 1, 130, 208},
  { "cs5n3p08", 271, 256, 1, 130, 208},
  { "cs8n2c08", 149, 226, 1, 116, 212},
  { "cs8n3p08", 256, 256, 1, 116, 212},
  { "ct0n0g04", 273, 271, 1, 232, 578},
  { "ct1n0g04", 792, 271, 1, 232, 578},
  { "ctzn0g04", 753, 271, 1, 232, 578},
  { "f00n0g08", 319, 312, 1, 288, 240},
  { "f00n2c08", 2475, 706, 0, 980, 400},
  { "f01n0g08", 321, 246, 1, 262, 188},
  { "f01n2c08", 1180, 657, 0, 874, 326},
  { "f02n0g08", 355, 289, 1, 270, 230},
  { "f02n2c08", 1729, 696, 0, 944, 390},
  { "f03n0g08", 389, 292, 1, 270, 226},
  { "f03n2c08", 1291, 697, 0, 962, 382},
  { "f04n0g08", 269, 273, 1, 266, 208},
  { "f04n2c08", 985, 672, 0, 898, 328},
  { "g03n0g16", 345, 273, 1, 280, 298},
  { "g03n2c08", 370, 396, 1, 374, 340},
  { "g03n3p04", 214, 214, 1, 156, 316},
  { "g04n0g16", 363, 287, 1, 276, 298},
  { "g04n2c08", 377, 399, 1, 370, 340},
  { "g04n3p04", 219, 219, 1, 156, 324},
  { "g05n0g16", 339, 275, 1, 276, 280},
  { "g05n2c08", 350, 402, 1, 366, 316},
  { "g05n3p04", 206, 206, 1, 146, 306},
  { "g07n0g16", 321, 261, 1, 254, 262},
  { "g07n2c08", 340, 401, 1, 368, 318},
  { "g07n3p04", 207, 207, 1, 148, 312},
  { "g10n0g16", 262, 210, 1, 250, 258},
  { "g10n2c08", 285, 403, 1, 360, 316},
  { "g10n3p04", 214, 214, 1, 148, 306},
  { "g25n0g16", 383, 305, 1, 270, 256},
  { "g25n2c08", 405, 399, 1, 356, 290},
  { "g25n3p04", 215, 215, 1, 150, 300},
  { "oi1n0g16", 167, 152, 1, 458, 150},
  { "oi1n2c16", 302, 274, 1, 134, 196},
  { "oi2n0g16", 179, 152, 1, 458, 150},
  { "oi2n2c16", 314, 274, 1, 134, 196},
  { "oi4n0g16", 203, 152, 1, 458, 150},
  { "oi4n2c16", 338, 274, 1, 134, 196},
  { "oi9n0g16", 1283, 152, 1, 458, 150},
  { "oi9n2c16", 3038, 274, 1, 134, 196},
  { "pp0n2c16", 962, 274, 1, 134, 196},
  { "pp0n6a08", 818, 158, 1, 108, 1232},
  { "ps1n0g08", 1477, 132, 1, 582, 156},
  { "ps1n2c16", 1641, 274, 1, 134, 196},
  { "ps2n0g08", 2341, 132, 1, 582, 156},
  { "ps2n2c16", 2505, 274, 1, 134, 196},
  { "s01i3p01", 113, 98, 1, 36, 72},
  { "s01n3p01", 113, 98, 1, 36, 72},
  { "s02i3p01", 114, 99, 1, 36, 98},
  { "s02n3p01", 115, 100, 1, 36, 98},
  { "s03i3p01", 118, 103, 1, 40, 82},
  { "s03n3p01", 120, 105, 1, 40, 82},
  { "s04i3p01", 126, 111, 1, 42, 90},
  { "s04n3p01", 121, 106, 1, 42, 90},
  { "s05i3p02", 134, 119, 1, 64, 112},
  { "s05n3p02", 129, 114, 1, 64, 112},
  { "s06i3p02", 143, 128, 1, 58, 122},
  { "s06n3p02", 131, 116, 1, 58, 122},
  { "s07i3p02", 149, 134, 1, 66, 154},
  { "s07n3p02", 138, 123, 1, 66, 154},
  { "s08i3p02", 149, 134, 1, 68, 130},
  { "s08n3p02", 139, 124, 1, 68, 130},
  { "s09i3p02", 147, 132, 1, 70, 122},
  { "s09n3p02", 143, 128, 1, 70, 122},
  { "s32i3p04", 355, 340, 1, 166, 680},
  { "s32n3p04", 263, 248, 1, 166, 680},
  { "s33i3p04", 385, 370, 1, 230, 914},
  { "s33n3p04", 329, 314, 1, 230, 914},
  { "s34i3p04", 349, 332, 1, 154, 846},
  { "s34n3p04", 248, 229, 1, 154, 846},
  { "s35i3p04", 399, 384, 1, 242, 972},
  { "s35n3p04", 338, 313, 1, 242, 972},
  { "s36i3p04", 356, 339, 1, 166, 1004},
  { "s36n3p04", 258, 240, 1, 166, 1004},
  { "s37i3p04", 393, 378, 1, 242, 936},
  { "s37n3p04", 336, 317, 1, 242, 936},
  { "s38i3p04", 357, 339, 1, 154, 940},
  { "s38n3p04", 245, 228, 1, 154, 940},
  { "s39i3p04", 420, 405, 1, 256, 1066},
  { "s39n3p04", 352, 336, 1, 256, 1066},
  { "s40i3p04", 357, 340, 1, 164, 950},
  { "s40n3p04", 256, 237, 1, 164, 950},
  { "tbbn1g04", 419, 405, 1, 356, 1408},
  { "tbbn2c16", 1994, 1095, 1, 860, 1476},
  { "tbbn3p08", 1128, 1095, 1, 860, 1476},
  { "tbgn2c16", 1994, 1095, 1, 860, 1476},
  { "tbgn3p08", 1128, 1095, 1, 860, 1476},
  { "tbrn2c08", 1347, 1095, 1, 860, 1476},
  { "tbwn1g16", 1146, 582, 1, 556, 1390},
  { "tbwn3p08", 1131, 1095, 1, 860, 1476},
  { "tbyn3p08", 1131, 1095, 1, 860, 1476},
  { "tp0n1g08", 689, 568, 1, 546, 338},
  { "tp0n2c08", 1311, 875, 0, 854, 424},
  { "tp0n3p08", 1120, 875, 0, 854, 424},
  { "tp1n3p08", 1115, 1095, 1, 860, 1476},
  { "z00n2c08", 3172, 224, 1, 144, 198},
  { "z03n2c08", 232, 224, 1, 144, 198},
  { "z06n2c08", 224, 224, 1, 144, 198},
  { "z09n2c08", 224, 224, 1, 144, 198},
};


// These images were obtained from
// http://entropymine.com/jason/testbed/pngtrans/
// Naming scheme: tr-tX-Yp[B|b] where:
//   X is the sequential image number as presented at the URL above,
//   Y is the number of bits per pixel (8-bit palette or 32/64-bit RGBA),
//   B denotes the image contains background color, and
//   b denotes the image does not contain background color.
ImageCompressionInfo kImagesWithAlpha[] = {
  { "tr-t1-8pB", 1152, 0, 0, 304, 6154},
  { "tr-t2-8pb", 1136, 0, 0, 304, 6154},
  { "tr-t3-32pB", 288, 0, 0, 304, 6154},
  { "tr-t4-32pb", 270, 0, 0, 304, 6154},
  { "tr-t5-64pB", 336, 0, 0, 304, 6154},
  { "tr-t6-64pb", 318, 0, 0, 304, 6154},
};

const char* kInvalidFiles[] = {
  "nosuchfile",
  "emptyfile",
  "x00n0g01",
  "xcrn0g04",
  "xlfn0g04",
};

const size_t kValidImageCount = arraysize(kValidImages);
const size_t kImagesWithAlphaCount = arraysize(kImagesWithAlpha);
const size_t kInvalidFileCount = arraysize(kInvalidFiles);

void ReadImageToString(const std::string& dir,
                       const char* file_name,
                       const char* ext,
                       std::string* dest) {
  const std::string path = dir + file_name + '.' + ext;
  pagespeed_testing::ReadFileToString(path, dest);
}

void ReadPngSuiteFileToString(const char* file_name, std::string* dest) {
  ReadImageToString(kPngSuiteTestDir, file_name, "png", dest);
}

void WriteStringToFile(const std::string &file_name, std::string &src) {
  LOG(INFO) << "Filename: " << file_name;
  const std::string path = "/tmp/image_converter_test/" + file_name;
  std::ofstream stream;
  stream.open(path.c_str(), std::ofstream::out | std::ofstream::binary);
  stream.write(src.c_str(), src.size());
  stream.close();
}

TEST(ImageConverterTest, OptimizePngOrConvertToJpeg_invalidPngs) {
  PngReader png_struct_reader;
  pagespeed::image_compression::JpegCompressionOptions options;
  for (size_t i = 0; i < kInvalidFileCount; i++) {
    std::string in, out;
    bool is_out_png;
    ReadPngSuiteFileToString(kInvalidFiles[i], &in);
    ASSERT_FALSE(ImageConverter::OptimizePngOrConvertToJpeg(
        png_struct_reader, in, options, &out, &is_out_png));
  }
}

TEST(ImageConverterTest, OptimizePngOrConvertToJpeg) {
  PngReader png_struct_reader;
  pagespeed::image_compression::JpegCompressionOptions options;
  // We are using default lossy options for conversion.
  options.lossy = true;
  options.progressive = false;
  for (size_t i = 0; i < kValidImageCount; i++) {
    std::string in, out;
    bool is_out_png;
    ReadPngSuiteFileToString(kValidImages[i].filename, &in);
    ASSERT_TRUE(ImageConverter::OptimizePngOrConvertToJpeg(
        png_struct_reader, in, options, &out, &is_out_png));
    // Verify that the size matches.
    EXPECT_EQ(kValidImages[i].compressed_size, out.size())
        << "size mismatch for " << kValidImages[i].filename;
    // Verify that out put image type matches.

    EXPECT_EQ(kValidImages[i].is_png, is_out_png)
        << "image type mismatch for " << kValidImages[i].filename;

    // Uncomment the line below for debugging
    // WriteStringToFile(std::string("icj-") + kValidImages[i].filename, out);
  }
}

TEST(ImageConverterTest, ConvertPngToWebp_invalidPngs) {
  PngReader png_struct_reader;
  WebpConfiguration webp_config;

  for (size_t i = 0; i < kInvalidFileCount; i++) {
    std::string in, out;
    ReadPngSuiteFileToString(kInvalidFiles[i], &in);
    ASSERT_FALSE(ImageConverter::ConvertPngToWebp(
        png_struct_reader, in, webp_config, &out));
  }
}

TEST(ImageConverterTest, ConvertPngToWebp) {
  PngReader png_struct_reader;

  WebpConfiguration lossless_config;
  WebpConfiguration lossy_config;

  lossy_config.lossless = false;
  lossy_config.quality = 50;
  lossy_config.method = 0;  // Quality vs speed: 0 is fastest/worst.
  lossy_config.alpha_quality = 50;

  for (size_t i = 0; i < kValidImageCount; i++) {
    std::string in, out;
    ImageCompressionInfo image = kValidImages[i];
    ReadPngSuiteFileToString(image.filename, &in);

    // First test with default config (lossless).
    if (image.webp_lossless_size > 0) {
      EXPECT_TRUE(ImageConverter::ConvertPngToWebp(
          png_struct_reader, in, lossless_config, &out));

      // Verify that the size matches.
      EXPECT_EQ(image.webp_lossless_size, out.size())
          << "size mismatch for " << image.filename;

    } else {
      EXPECT_FALSE(ImageConverter::ConvertPngToWebp(
          png_struct_reader, in, lossless_config, &out)) << image.filename;
    }
    // Uncomment the line below for debugging.
    // WriteStringToFile(std::string("icw-") + image.filename + ".webp", out);

    // Now test with non-default config (lossy).
    out.clear();

    if (image.webp_lossy_q50m0aq50_size > 0) {
      EXPECT_TRUE(ImageConverter::ConvertPngToWebp(
          png_struct_reader, in, lossy_config, &out));

      // Verify that the size matches.
      EXPECT_EQ(image.webp_lossy_q50m0aq50_size, out.size())
          << "size mismatch for " << image.filename;

    } else {
      EXPECT_FALSE(ImageConverter::ConvertPngToWebp(
          png_struct_reader, in, lossy_config, &out)) << image.filename;
    }
    // Uncomment the line below for debugging
    // WriteStringToFile(std::string("icwl-") + image.filename + ".webp", out);
  }
}

TEST(ImageConverterTest, ConvertPngToWebp_withalpha) {
  PngReader png_struct_reader;

  WebpConfiguration lossless_config;
  WebpConfiguration lossy_config;
  lossy_config.lossless = false;
  lossy_config.quality = 50;
  lossy_config.method = 0;
  lossy_config.alpha_quality = 50;

  for (size_t i = 0; i < kImagesWithAlphaCount; i++) {
    std::string in, out;
    ImageCompressionInfo image = kImagesWithAlpha[i];
    ReadPngSuiteFileToString(image.filename, &in);

    // First test with default config (lossless).
    if (image.webp_lossless_size > 0) {
      EXPECT_TRUE(ImageConverter::ConvertPngToWebp(
          png_struct_reader, in, lossless_config, &out));

      // Verify that the size matches.
      EXPECT_EQ(image.webp_lossless_size, out.size())
          << "size mismatch for " << image.filename;

    } else {
      EXPECT_FALSE(ImageConverter::ConvertPngToWebp(
          png_struct_reader, in, lossless_config, &out)) << image.filename;
    }
    // Uncomment the line below for debugging
    // WriteStringToFile(std::string("icw-") + image.filename + ".webp", out);

    // Now test with non-default config (lossy).
    out.clear();
    if (image.webp_lossless_size > 0) {
      EXPECT_TRUE(ImageConverter::ConvertPngToWebp(
          png_struct_reader, in, lossy_config, &out));

      // Verify that the size matches.
      EXPECT_EQ(image.webp_lossy_q50m0aq50_size, out.size())
          << "size mismatch for " << image.filename;

    } else {
      EXPECT_FALSE(ImageConverter::ConvertPngToWebp(
          png_struct_reader, in, lossy_config, &out)) << image.filename;
    }
    // Uncomment the line below for debugging
    // WriteStringToFile(std::string("icwl-") + image.filename + ".webp", out);
  }
}

TEST(ImageConverterTest, GetSmallestOfPngJpegWebp) {
  PngReader png_struct_reader;
  const double kMinJpegSavingsRatio = 0.8;
  const double kMinWebpSavingsRatio = 0.8;

  pagespeed::image_compression::JpegCompressionOptions jpeg_options;
  // We are using default lossy options for conversion.
  jpeg_options.lossy = true;
  jpeg_options.progressive = false;

  WebpConfiguration lossy_config;
  lossy_config.lossless = false;
  lossy_config.quality = 50;
  lossy_config.method = 0;
  lossy_config.alpha_quality = 50;

  for (size_t i = 0; i < kValidImageCount; i++) {
    std::string in, out;
    ImageCompressionInfo image = kValidImages[i];
    ReadPngSuiteFileToString(image.filename, &in);

    ImageConverter::ImageType expected_image_type;
    size_t expected_image_size;
    if (image.is_png &&
        ((image.webp_lossless_size == 0) ||
         (image.compressed_size < image.webp_lossless_size)) &&
        ((image.webp_lossy_q50m0aq50_size == 0) ||
         (image.compressed_size * kMinJpegSavingsRatio <
          image.webp_lossy_q50m0aq50_size))) {
      expected_image_type = ImageConverter::IMAGE_PNG;
      expected_image_size = image.compressed_size;
    } else if (!image.is_png &&
               ((image.webp_lossless_size == 0) ||
                (image.compressed_size <
                 image.webp_lossless_size * kMinWebpSavingsRatio)) &&
               ((image.webp_lossy_q50m0aq50_size == 0) ||
                (image.compressed_size < image.webp_lossy_q50m0aq50_size))) {
      expected_image_type = ImageConverter::IMAGE_JPEG;
      expected_image_size = image.compressed_size;
    } else {
      expected_image_type = ImageConverter::IMAGE_WEBP;
      expected_image_size =
          ((image.webp_lossless_size > 0) &&
           (image.webp_lossless_size * kMinWebpSavingsRatio <
            image.webp_lossy_q50m0aq50_size)) ?
          image.webp_lossless_size : image.webp_lossy_q50m0aq50_size;
      }
    EXPECT_EQ(expected_image_type,
              ImageConverter::GetSmallestOfPngJpegWebp(
                  png_struct_reader, in, &jpeg_options,
                  &lossy_config, &out)) << image.filename;
    EXPECT_EQ(expected_image_size, out.size()) << image.filename;
  }
}

}  // namespace
