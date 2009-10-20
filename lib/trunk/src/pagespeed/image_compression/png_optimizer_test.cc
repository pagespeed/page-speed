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

#include "pagespeed/image_compression/png_optimizer.h"

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

const char *kValidFiles[] = {
  "basi0g01.png",
  "basi0g02.png",
  "basi0g04.png",
  "basi0g08.png",
  "basi0g16.png",
  "basi2c08.png",
  "basi2c16.png",
  "basi3p01.png",
  "basi3p02.png",
  "basi3p04.png",
  "basi3p08.png",
  "basi4a08.png",
  "basi4a16.png",
  "basi6a08.png",
  "basi6a16.png",
  "basn0g01.png",
  "basn0g02.png",
  "basn0g04.png",
  "basn0g08.png",
  "basn0g16.png",
  "basn2c08.png",
  "basn2c16.png",
  "basn3p01.png",
  "basn3p02.png",
  "basn3p04.png",
  "basn3p08.png",
  "basn4a08.png",
  "basn4a16.png",
  "basn6a08.png",
  "basn6a16.png",
  "bgai4a08.png",
  "bgai4a16.png",
  "bgan6a08.png",
  "bgan6a16.png",
  "bgbn4a08.png",
  "bggn4a16.png",
  "bgwn6a08.png",
  "bgyn6a16.png",
  "ccwn2c08.png",
  "ccwn3p08.png",
  "cdfn2c08.png",
  "cdhn2c08.png",
  "cdsn2c08.png",
  "cdun2c08.png",
  "ch1n3p04.png",
  "ch2n3p08.png",
  "cm0n0g04.png",
  "cm7n0g04.png",
  "cm9n0g04.png",
  "cs3n2c16.png",
  "cs3n3p08.png",
  "cs5n2c08.png",
  "cs5n3p08.png",
  "cs8n2c08.png",
  "cs8n3p08.png",
  "ct0n0g04.png",
  "ct1n0g04.png",
  "ctzn0g04.png",
  "f00n0g08.png",
  "f00n2c08.png",
  "f01n0g08.png",
  "f01n2c08.png",
  "f02n0g08.png",
  "f02n2c08.png",
  "f03n0g08.png",
  "f03n2c08.png",
  "f04n0g08.png",
  "f04n2c08.png",
  "g03n0g16.png",
  "g03n2c08.png",
  "g03n3p04.png",
  "g04n0g16.png",
  "g04n2c08.png",
  "g04n3p04.png",
  "g05n0g16.png",
  "g05n2c08.png",
  "g05n3p04.png",
  "g07n0g16.png",
  "g07n2c08.png",
  "g07n3p04.png",
  "g10n0g16.png",
  "g10n2c08.png",
  "g10n3p04.png",
  "g25n0g16.png",
  "g25n2c08.png",
  "g25n3p04.png",
  "oi1n0g16.png",
  "oi1n2c16.png",
  "oi2n0g16.png",
  "oi2n2c16.png",
  "oi4n0g16.png",
  "oi4n2c16.png",
  "oi9n0g16.png",
  "oi9n2c16.png",
  "pp0n2c16.png",
  "pp0n6a08.png",
  "ps1n0g08.png",
  "ps1n2c16.png",
  "ps2n0g08.png",
  "ps2n2c16.png",
  "s01i3p01.png",
  "s01n3p01.png",
  "s02i3p01.png",
  "s02n3p01.png",
  "s03i3p01.png",
  "s03n3p01.png",
  "s04i3p01.png",
  "s04n3p01.png",
  "s05i3p02.png",
  "s05n3p02.png",
  "s06i3p02.png",
  "s06n3p02.png",
  "s07i3p02.png",
  "s07n3p02.png",
  "s08i3p02.png",
  "s08n3p02.png",
  "s09i3p02.png",
  "s09n3p02.png",
  "s32i3p04.png",
  "s32n3p04.png",
  "s33i3p04.png",
  "s33n3p04.png",
  "s34i3p04.png",
  "s34n3p04.png",
  "s35i3p04.png",
  "s35n3p04.png",
  "s36i3p04.png",
  "s36n3p04.png",
  "s37i3p04.png",
  "s37n3p04.png",
  "s38i3p04.png",
  "s38n3p04.png",
  "s39i3p04.png",
  "s39n3p04.png",
  "s40i3p04.png",
  "s40n3p04.png",
  "tbbn1g04.png",
  "tbbn2c16.png",
  "tbbn3p08.png",
  "tbgn2c16.png",
  "tbgn3p08.png",
  "tbrn2c08.png",
  "tbwn1g16.png",
  "tbwn3p08.png",
  "tbyn3p08.png",
  "tp0n1g08.png",
  "tp0n2c08.png",
  "tp0n3p08.png",
  "tp1n3p08.png",
  "z00n2c08.png",
  "z03n2c08.png",
  "z06n2c08.png",
  "z09n2c08.png",
};

const char *kInvalidFiles[] = {
  "nosuchfile.png",
  "emptyfile.png",
  "x00n0g01.png",
  "xcrn0g04.png",
  "xlfn0g04.png",
};

const size_t kValidFileCount = sizeof(kValidFiles) / sizeof(kValidFiles[0]);
const size_t kInvalidFileCount =
    sizeof(kInvalidFiles) / sizeof(kInvalidFiles[0]);

TEST(PngOptimizerTest, ValidPngs) {
  for (int i = 0; i < kValidFileCount; i++) {
    std::string in, out;
    ReadFileToString(kValidFiles[i], &in);
    ASSERT_TRUE(PngOptimizer::OptimizePng(in, &out));
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
      ReadFileToString(kValidFiles[i], &in);
      ASSERT_TRUE(PngOptimizer::OptimizePng(in, &out));
    }
  }
}

}  // namespace
