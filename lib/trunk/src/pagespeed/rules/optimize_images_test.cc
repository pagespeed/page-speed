// Copyright 2009 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <fstream>
#include <string>

#include "base/memory/scoped_ptr.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/rules/optimize_images.h"
#include "pagespeed/testing/pagespeed_test.h"

using pagespeed::rules::OptimizeImages;
using pagespeed::PagespeedInput;
using pagespeed::Resource;
using pagespeed::Result;
using pagespeed::Results;
using pagespeed::ResultProvider;
using pagespeed::ResultVector;
using pagespeed::RuleResults;
using pagespeed_testing::ReadFileToString;

namespace {

// The JPEG_TEST_DIR_PATH and PNG_TEST_DIR_PATH macros are set by the gyp
// target that builds this file.
const std::string kJpegTestDir = IMAGE_TEST_DIR_PATH "jpeg/";
const std::string kPngSuiteTestDir = IMAGE_TEST_DIR_PATH "pngsuite/";

class OptimizeImagesTest :
      public ::pagespeed_testing::PagespeedRuleTest<OptimizeImages> {
 protected:
  void AddJpegResource(const std::string &url,
                       const std::string &content_type,
                       const std::string &file_name) {
    std::string body;
    ASSERT_TRUE(ReadFileToString(kJpegTestDir + file_name, &body));
    AddTestResource(url, content_type, body);
  }

  void AddPngResource(const std::string &url,
                      const std::string &content_type,
                      const std::string &file_name) {
    std::string body;
    ASSERT_TRUE(ReadFileToString(kPngSuiteTestDir + file_name, &body));
    AddTestResource(url, content_type, body);
  }

  void AddTestResource(const std::string &url,
                       const std::string &content_type,
                       const std::string &body) {
    Resource* resource = New200Resource(url);
    resource->AddResponseHeader("Content-Type", content_type);
    resource->SetResponseBody(body);
  }
};

TEST_F(OptimizeImagesTest, BasicJpg) {
  AddJpegResource("http://www.example.com/foo.jpg",
                  "image/jpg",
                  "test420.jpg");
  CheckOneUrlViolation("http://www.example.com/foo.jpg");
}

TEST_F(OptimizeImagesTest, BasicJpeg) {
  AddJpegResource("http://www.example.com/foo.jpeg",
                  "image/jpeg",
                  "test411.jpg");
  CheckOneUrlViolation("http://www.example.com/foo.jpeg");
}

TEST_F(OptimizeImagesTest, BasicPng) {
  AddPngResource("http://www.example.com/foo.png",
                 "image/png",
                 "basi3p02.png");
  CheckOneUrlViolation("http://www.example.com/foo.png");
}

TEST_F(OptimizeImagesTest, UnknownImageTypeDoesNotGetOptimized) {
  AddJpegResource("http://www.example.com/foo.xyz",
                  "image/xyz",
                  "testgray.jpg");
  CheckNoViolations();
}

TEST_F(OptimizeImagesTest, WrongContentTypeDoesNotGetOptimizedJpeg) {
  AddJpegResource("http://www.example.com/foo.jpeg",
                  "application/x-foo-bar-baz",
                  "testgray.jpg");
  CheckNoViolations();
}

TEST_F(OptimizeImagesTest, WrongContentTypeDoesNotGetOptimizedPng) {
  AddPngResource("http://www.example.com/foo.png",
                 "application/x-foo-bar-baz",
                 "basi0g01.png");
  CheckNoViolations();
}

TEST_F(OptimizeImagesTest, AlreadyOptimizedJpeg) {
  AddJpegResource("http://www.example.com/foo.jpeg",
                  "image/jpeg",
                  "already_optimized.jpg");
  CheckNoViolations();
}

TEST_F(OptimizeImagesTest, AlreadyOptimizedPng) {
  AddPngResource("http://www.example.com/foo.png",
                 "image/png",
                 "already_optimized.png");
  CheckNoViolations();
}

TEST_F(OptimizeImagesTest, ErrorJpeg) {
  AddJpegResource("http://www.example.com/foo.jpeg",
                  "image/jpeg",
                  "corrupt.jpg");
  CheckError();
}

TEST_F(OptimizeImagesTest, ErrorPng) {
  AddPngResource("http://www.example.com/foo.png",
                 "image/png",
                 "x00n0g01.png");
  CheckError();
}

TEST_F(OptimizeImagesTest, Format) {
  AddJpegResource("http://www.example.com/foo.jpg",
                  "image/jpg",
                  "test420.jpg");
  CheckOneUrlViolation("http://www.example.com/foo.jpg");
  ASSERT_EQ(
      "Properly formatting and compressing images can save "
      "many bytes of data.\n"
      "Optimize the following images"
      "<https://developers.google.com/speed/docs/insights/OptimizeImages> "
      "to reduce their size by 2.5KiB (41% reduction).\n  "
      "Losslessly compressing http://www.example.com/foo.jpg could save "
      "2.5KiB (41% reduction).\n", FormatResults());
}

}  // namespace
