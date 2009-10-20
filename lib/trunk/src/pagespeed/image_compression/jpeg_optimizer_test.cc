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

// Author: Bryan McQuade, Matthew Steele

#include <fstream>
#include <string>
#include <vector>

#include "pagespeed/image_compression/jpeg_optimizer.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace {

// The JPEG_TEST_DIR_PATH macro is set by the gyp target that builds this file.
const std::string kJpegTestDir = JPEG_TEST_DIR_PATH;

const char *kValidFiles[] = {
  "sjpeg1.jpg",
  "sjpeg2.jpg",
  "sjpeg3.jpg",
  "sjpeg4.jpg",
  "sjpeg5.jpg",
  "sjpeg6.jpg",
  "test411.jpg",
  "test420.jpg",
  "test422.jpg",
  "testgray.jpg",
};

const char *kInvalidFiles[] = {
  "notajpeg.png",  // A png.
  "notajpeg.gif",  // A gif.
  "emptyfile.jpg", // A zero-byte file.
  "corrupt.jpg",   // Invalid huffman code in the image data section.
};

// Given one of the above file names, read the contents of the file into the
// given destination string.

void ReadFileToString(const std::string &file_name, std::string *dest) {
  const std::string path = kJpegTestDir + file_name;
  std::ifstream file_stream;
  file_stream.open(path.c_str(), std::ifstream::in | std::ifstream::binary);
  dest->assign(std::istreambuf_iterator<char>(file_stream),
               std::istreambuf_iterator<char>());
  file_stream.close();
}

void WriteStringToFile(const std::string &file_name, std::string &src) {
  const std::string path = kJpegTestDir + file_name;
  std::ofstream stream;
  stream.open(path.c_str(), std::ofstream::out | std::ofstream::binary);
  stream.write(src.c_str(), src.size());
  stream.close();
}

const size_t kValidFileCount = sizeof(kValidFiles) / sizeof(kValidFiles[0]);
const size_t kInvalidFileCount =
    sizeof(kInvalidFiles) / sizeof(kInvalidFiles[0]);

TEST(JpegOptimizerTest, ValidJpegs) {
  for (int i = 0; i < kValidFileCount; ++i) {
    std::string src_data;
    ReadFileToString(kValidFiles[i], &src_data);
    std::string dest_data;
    pagespeed::image_compression::JpegOptimizer optimizer;
    ASSERT_TRUE(optimizer.CreateOptimizedJpeg(src_data, &dest_data));
    ASSERT_GT(dest_data.size(), 0);

    // Uncomment this next line for debugging:
    //WriteStringToFile(std::string("z") + kValidFiles[i], dest_data);

    // You'd think we'd want this next line, but it's not always true.  At
    // some point we should look into why libjpeg sometimes makes it bigger.
    //ASSERT_LE(dest_data.size(), src_data.size());
  }
}

TEST(JpegOptimizerTest, InvalidJpegs) {
  for (int i = 0; i < kInvalidFileCount; ++i) {
    std::string src_data;
    ReadFileToString(kInvalidFiles[i], &src_data);
    std::string dest_data;
    pagespeed::image_compression::JpegOptimizer optimizer;
    ASSERT_FALSE(optimizer.CreateOptimizedJpeg(src_data, &dest_data));
  }
}

// Test that after reading an invalid jpeg, the reader cleans its state so that
// it can read a correct jpeg again.
TEST(JpegOptimizerTest, CleanupAfterReadingInvalidJpeg) {
  // Compress each input image with a reinitialized JpegOptimizer.
  // We will compare these files with the output we get from
  // a JpegOptimizer that had an error.
  std::vector<std::string> correctly_compressed;
  for (int i = 0; i < kValidFileCount; ++i) {
    std::string src_data;
    ReadFileToString(kValidFiles[i], &src_data);
    correctly_compressed.push_back("");
    std::string &dest_data = correctly_compressed.back();
    pagespeed::image_compression::JpegOptimizer optimizer;
    ASSERT_TRUE(optimizer.CreateOptimizedJpeg(src_data, &dest_data));
  }

  // The invalid files are all invalid in different ways, and we want to cover
  // all the ways jpeg decoding can fail.  So, we want at least as many valid
  // images as invalid ones.
  ASSERT_GE(kValidFileCount, kInvalidFileCount);

  for (int i = 0; i < kInvalidFileCount; ++i) {
    std::string invalid_src_data;
    ReadFileToString(kInvalidFiles[i], &invalid_src_data);
    std::string invalid_dest_data;

    std::string valid_src_data;
    ReadFileToString(kValidFiles[i], &valid_src_data);
    std::string valid_dest_data;

    pagespeed::image_compression::JpegOptimizer optimizer;
    ASSERT_FALSE(optimizer.CreateOptimizedJpeg(invalid_src_data,
                                               &invalid_dest_data));
    ASSERT_TRUE(optimizer.CreateOptimizedJpeg(valid_src_data,
                                              &valid_dest_data));

    // Diff the jpeg created by CreateOptimizedJpeg() with the one created
    // with a reinitialized JpegOptimizer.
    ASSERT_EQ(valid_dest_data, correctly_compressed.at(i));
  }
}

}  // namespace
