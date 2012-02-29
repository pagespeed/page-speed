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

// Author: Satyanarayana Manyam

#include <string>

#include "base/basictypes.h"

#include "pagespeed/image_compression/jpeg_utils.h"
#include "pagespeed/testing/pagespeed_test.h"

// DO NOT INCLUDE LIBJPEG HEADERS HERE. Doing so causes build errors
// on Windows. If you need to call out to libjpeg, please add helper
// methods in jpeg_optimizer_test_helper.h.

namespace {

using pagespeed::image_compression::JpegUtils;

// The JPEG_TEST_DIR_PATH macro is set by the gyp target that builds this file.
const std::string kJpegTestDir = IMAGE_TEST_DIR_PATH "jpeg/";
const char* kColorJpegFile = "sjpeg2.jpg";
const char* kGreyScaleJpegFile = "testgray.jpg";
const char* kEmptyJpegFile = "emptyfile.jpg";

// Given one of the above file names, read the contents of the file into the
// given destination string.

void ReadJpegToString(const std::string &file_name, std::string *dest) {
  const std::string path = kJpegTestDir + file_name;
  pagespeed_testing::ReadFileToString(path, dest);
}

TEST(JpegUtilsTest, GetImageQualityFromImage) {
  std::string src_data;
  ReadJpegToString(kGreyScaleJpegFile, &src_data);
  EXPECT_EQ(85, JpegUtils::GetImageQualityFromImage(src_data));

  src_data.clear();
  ReadJpegToString(kColorJpegFile, &src_data);
  EXPECT_EQ(75, JpegUtils::GetImageQualityFromImage(src_data));

  src_data.clear();
  ReadJpegToString(kEmptyJpegFile, &src_data);
  EXPECT_EQ(-1, JpegUtils::GetImageQualityFromImage(src_data));
}

}  // namespace
