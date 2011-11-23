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
using pagespeed::image_compression::PngReader;

// The *_TEST_DIR_PATH macro is set by the gyp target that builds this file.
const std::string kPngSuiteTestDir = IMAGE_TEST_DIR_PATH "pngsuite/";

struct ImageCompressionInfo {
  const char* filename;
  size_t original_size;
  size_t compressed_size;
  bool is_png;
};

ImageCompressionInfo kValidImages[] {
  { "basi0g01", 217, 217, 1},
  { "basi0g02", 154, 154, 1},
  { "basi0g04", 247, 247, 1},
  { "basi0g08", 254, 241, 0},
  { "basi0g16", 299, 291, 0},
  { "basi2c08", 315, 452, 0},
  { "basi2c16", 595, 419, 0},
  { "basi3p01", 132, 132, 1},
  { "basi3p02", 193, 178, 1},
  { "basi3p04", 327, 312, 1},
  { "basi3p08", 1527, 565, 0},
  { "basi4a08", 214, 1450, 1},
  { "basi4a16", 2855, 1980, 1},
  { "basi6a08", 361, 1591, 1},
  { "basi6a16", 4180, 4421, 1},
  { "basn0g01", 164, 164, 1},
  { "basn0g02", 104, 104, 1},
  { "basn0g04", 145, 145, 1},
  { "basn0g08", 138, 241, 0},
  { "basn0g16", 167, 291, 0},
  { "basn2c08", 145, 452, 0},
  { "basn2c16", 302, 419, 0},
  { "basn3p01", 112, 112, 1},
  { "basn3p02", 146, 131, 1},
  { "basn3p04", 216, 201, 1},
  { "basn3p08", 1286, 565, 0},
  { "basn4a08", 126, 1433, 1},
  { "basn4a16", 2206, 1185, 1},
  { "basn6a08", 184, 1435, 1},
  { "basn6a16", 3435, 4190, 1},
  { "bgai4a08", 214, 1450, 1},
  { "bgai4a16", 2855, 1980, 1},
  { "bgan6a08", 184, 1435, 1},
  { "bgan6a16", 3435, 4190, 1},
  { "bgbn4a08", 140, 1433, 1},
  { "bggn4a16", 2220, 1185, 1},
  { "bgwn6a08", 202, 1435, 1},
  { "bgyn6a16", 3453, 4190, 1},
  { "ccwn2c08", 1514, 764, 0},
  { "ccwn3p08", 1554, 779, 0},
  { "cdfn2c08", 404, 491, 0},
  { "cdhn2c08", 344, 491, 1},
  { "cdsn2c08", 232, 258, 1},
  { "cdun2c08", 724, 864, 0},
  { "ch1n3p04", 258, 201, 1},
  { "ch2n3p08", 1810, 565, 0},
  { "cm0n0g04", 292, 274, 1},
  { "cm7n0g04", 292, 274, 1},
  { "cm9n0g04", 292, 274, 1},
  { "cs3n2c16", 214, 204, 1},
  { "cs3n3p08", 259, 244, 1},
  { "cs5n2c08", 186, 256, 1},
  { "cs5n3p08", 271, 256, 1},
  { "cs8n2c08", 149, 256, 1},
  { "cs8n3p08", 256, 256, 1},
  { "ct0n0g04", 273, 274, 1},
  { "ct1n0g04", 792, 274, 1},
  { "ctzn0g04", 753, 274, 1},
  { "f00n0g08", 319, 319, 1},
  { "f00n2c08", 2475, 706, 0},
  { "f01n0g08", 321, 283, 1},
  { "f01n2c08", 1180, 657, 0},
  { "f02n0g08", 355, 297, 1},
  { "f02n2c08", 1729, 696, 0},
  { "f03n0g08", 389, 296, 1},
  { "f03n2c08", 1291, 697, 0},
  { "f04n0g08", 269, 281, 1},
  { "f04n2c08", 985, 672, 0},
  { "g03n0g16", 345, 308, 1},
  { "g03n2c08", 370, 490, 1},
  { "g03n3p04", 214, 214, 1},
  { "g04n0g16", 363, 310, 1},
  { "g04n2c08", 377, 493, 1},
  { "g04n3p04", 219, 219, 1},
  { "g05n0g16", 339, 306, 1},
  { "g05n2c08", 350, 488, 1},
  { "g05n3p04", 206, 206, 1},
  { "g07n0g16", 321, 305, 1},
  { "g07n2c08", 340, 488, 1},
  { "g07n3p04", 207, 207, 1},
  { "g10n0g16", 262, 306, 1},
  { "g10n2c08", 285, 495, 1},
  { "g10n3p04", 214, 214, 1},
  { "g25n0g16", 383, 305, 1},
  { "g25n2c08", 405, 470, 1},
  { "g25n3p04", 215, 215, 1},
  { "oi1n0g16", 167, 291, 0},
  { "oi1n2c16", 302, 419, 0},
  { "oi2n0g16", 179, 291, 0},
  { "oi2n2c16", 314, 419, 0},
  { "oi4n0g16", 203, 291, 0},
  { "oi4n2c16", 338, 419, 0},
  { "oi9n0g16", 1283, 291, 0},
  { "oi9n2c16", 3038, 419, 0},
  { "pp0n2c16", 962, 419, 0},
  { "pp0n6a08", 818, 3006, 1},
  { "ps1n0g08", 1477, 241, 0},
  { "ps1n2c16", 1641, 419, 0},
  { "ps2n0g08", 2341, 241, 0},
  { "ps2n2c16", 2505, 419, 0},
  { "s01i3p01", 113, 98, 1},
  { "s01n3p01", 113, 98, 1},
  { "s02i3p01", 114, 99, 1},
  { "s02n3p01", 115, 100, 1},
  { "s03i3p01", 118, 103, 1},
  { "s03n3p01", 120, 105, 1},
  { "s04i3p01", 126, 111, 1},
  { "s04n3p01", 121, 106, 1},
  { "s05i3p02", 134, 119, 1},
  { "s05n3p02", 129, 114, 1},
  { "s06i3p02", 143, 128, 1},
  { "s06n3p02", 131, 116, 1},
  { "s07i3p02", 149, 134, 1},
  { "s07n3p02", 138, 123, 1},
  { "s08i3p02", 149, 134, 1},
  { "s08n3p02", 139, 124, 1},
  { "s09i3p02", 147, 132, 1},
  { "s09n3p02", 143, 128, 1},
  { "s32i3p04", 355, 340, 1},
  { "s32n3p04", 263, 248, 1},
  { "s33i3p04", 385, 370, 1},
  { "s33n3p04", 329, 314, 1},
  { "s34i3p04", 349, 332, 1},
  { "s34n3p04", 248, 233, 1},
  { "s35i3p04", 399, 384, 1},
  { "s35n3p04", 338, 322, 1},
  { "s36i3p04", 356, 339, 1},
  { "s36n3p04", 258, 242, 1},
  { "s37i3p04", 393, 378, 1},
  { "s37n3p04", 336, 321, 1},
  { "s38i3p04", 357, 339, 1},
  { "s38n3p04", 245, 228, 1},
  { "s39i3p04", 420, 405, 1},
  { "s39n3p04", 352, 336, 1},
  { "s40i3p04", 357, 340, 1},
  { "s40n3p04", 256, 240, 1},
  { "tbbn1g04", 419, 405, 1},
  { "tbbn2c16", 1994, 1109, 1},
  { "tbbn3p08", 1128, 1110, 1},
  { "tbgn2c16", 1994, 1109, 1},
  { "tbgn3p08", 1128, 1110, 1},
  { "tbrn2c08", 1347, 1109, 1},
  { "tbwn1g16", 1146, 598, 1},
  { "tbwn3p08", 1131, 1110, 1},
  { "tbyn3p08", 1131, 1110, 1},
  { "tp0n1g08", 689, 584, 1},
  { "tp0n2c08", 1311, 875, 0},
  { "tp0n3p08", 1120, 875, 0},
  { "tp1n3p08", 1115, 1110, 1},
  { "z00n2c08", 3172, 423, 0},
  { "z03n2c08", 232, 423, 0},
  { "z06n2c08", 224, 423, 0},
  { "z09n2c08", 224, 423, 0},
};

const char* kInvalidFiles[] = {
  "nosuchfile",
  "emptyfile",
  "x00n0g01",
  "xcrn0g04",
  "xlfn0g04",
};

const size_t kValidImageCount = arraysize(kValidImages);
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
  const std::string path = "/home/satyanarayana/test/out/" + file_name;
  std::ofstream stream;
  stream.open(path.c_str(), std::ofstream::out | std::ofstream::binary);
  stream.write(src.c_str(), src.size());
  stream.close();
}

TEST(ImageConverterTest, OptimizePngOrConvertToJpeg_invalidPngs) {
  PngReader png_struct_reader;
  for (size_t i = 0; i < kInvalidFileCount; i++) {
    std::string in, out;
    bool is_out_png;
    ReadPngSuiteFileToString(kInvalidFiles[i], &in);
    ASSERT_FALSE(ImageConverter::OptimizePngOrConvertToJpeg(
        png_struct_reader, in, &out, &is_out_png));
  }
}

TEST(ImageConverterTest, OptimizePngOrConvertToJpeg) {
  PngReader png_struct_reader;
  for (size_t i = 0; i < kValidImageCount; i++) {
    std::string in, out;
    bool is_out_png;
    ReadPngSuiteFileToString(kValidImages[i].filename, &in);
    ASSERT_TRUE(ImageConverter::OptimizePngOrConvertToJpeg(
        png_struct_reader, in, &out, &is_out_png));
    // Verify that the size matches.
    EXPECT_EQ(out.size(), kValidImages[i].compressed_size)
        << "size mismatch for " << kValidImages[i].filename;
    // Verify that out put image type matches.

    EXPECT_EQ(is_out_png, kValidImages[i].is_png)
        << "image type mismatch for " << kValidImages[i].filename;

    // Uncomment below line for debugging
    //WriteStringToFile(std::string("ic") + kValidImages[i].filename, out);
  }
}

}  // namespace
