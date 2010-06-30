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

#include "base/scoped_ptr.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/rules/optimize_images.h"
#include "testing/gtest/include/gtest/gtest.h"

using pagespeed::rules::OptimizeImages;
using pagespeed::PagespeedInput;
using pagespeed::Resource;
using pagespeed::Result;
using pagespeed::Results;
using pagespeed::ResultProvider;
using pagespeed::ResultVector;

namespace {

// The JPEG_TEST_DIR_PATH and PNG_TEST_DIR_PATH macros are set by the gyp
// target that builds this file.
const std::string kJpegTestDir = IMAGE_TEST_DIR_PATH "jpeg/";
const std::string kPngSuiteTestDir = IMAGE_TEST_DIR_PATH "pngsuite/";

void ReadFileToString(const std::string &path, std::string *dest) {
  std::ifstream file_stream;
  file_stream.open(path.c_str(), std::ifstream::in | std::ifstream::binary);
  dest->assign(std::istreambuf_iterator<char>(file_stream),
               std::istreambuf_iterator<char>());
  file_stream.close();
  ASSERT_GT(static_cast<size_t>(0), dest->size());
}

class OptimizeImagesTest : public ::testing::Test {
 protected:

  virtual void SetUp() {
    input_.reset(new PagespeedInput);
  }

  virtual void TearDown() {
    input_.reset();
  }

  void AddJpegResource(const std::string &url,
                       const std::string &content_type,
                       const std::string &file_name) {
    std::string body;
    ReadFileToString(kJpegTestDir + file_name, &body);
    AddTestResource(url, content_type, body);
  }

  void AddPngResource(const std::string &url,
                      const std::string &content_type,
                      const std::string &file_name) {
    std::string body;
    ReadFileToString(kPngSuiteTestDir + file_name, &body);
    AddTestResource(url, content_type, body);
  }

  void AddTestResource(const std::string &url,
                       const std::string &content_type,
                       const std::string &body) {
    Resource* resource = new Resource;
    resource->SetRequestUrl(url);
    resource->SetRequestMethod("GET");
    resource->SetRequestProtocol("HTTP");
    resource->SetResponseStatusCode(200);
    resource->SetResponseProtocol("HTTP/1.1");
    resource->AddResponseHeader("Content-Type", content_type);
    resource->SetResponseBody(body);
    input_->AddResource(resource);
  }

  void CheckNoViolations() {
    CheckNoViolationsInternal(false);
    CheckNoViolationsInternal(true);
  }

  void CheckOneViolation(const std::string &url, int score) {
    CheckOneViolationInternal(url, false, score);
    CheckOneViolationInternal(url, true, score);
  }

  void CheckError() {
    CheckErrorInternal(false);
    CheckErrorInternal(true);
  }

 private:
  void CheckNoViolationsInternal(bool save_optimized_content) {
    OptimizeImages optimize(save_optimized_content);

    Results results;
    ResultProvider provider(optimize, &results);
    ASSERT_TRUE(optimize.AppendResults(*input_, &provider));
    ASSERT_EQ(results.results_size(), 0);
  }

  void CheckOneViolationInternal(const std::string &url,
                                 bool save_optimized_content,
                                 int score) {
    OptimizeImages optimize(save_optimized_content);

    Results results;
    ResultProvider provider(optimize, &results);
    ASSERT_TRUE(optimize.AppendResults(*input_, &provider));
    ASSERT_EQ(results.results_size(), 1);

    const Result& result = results.results(0);
    ASSERT_GT(result.savings().response_bytes_saved(), 0);
    ASSERT_EQ(result.resource_urls_size(), 1);
    ASSERT_EQ(result.resource_urls(0), url);

    ASSERT_EQ(save_optimized_content, result.has_optimized_content());

    ResultVector result_vector;
    result_vector.push_back(&result);
    ASSERT_EQ(score, optimize.ComputeScore(*input_->input_information(),
                                           result_vector));
  }

  void CheckErrorInternal(bool save_optimized_content) {
    OptimizeImages optimize(save_optimized_content);

    Results results;
    ResultProvider provider(optimize, &results);
    ASSERT_FALSE(optimize.AppendResults(*input_, &provider));
    ASSERT_EQ(results.results_size(), 0);
  }

  scoped_ptr<PagespeedInput> input_;
};

TEST_F(OptimizeImagesTest, BasicJpg) {
  AddJpegResource("http://www.example.com/foo.jpg",
                  "image/jpg",
                  "test420.jpg");
  CheckOneViolation("http://www.example.com/foo.jpg", 13);
}

TEST_F(OptimizeImagesTest, BasicJpeg) {
  AddJpegResource("http://www.example.com/foo.jpeg",
                  "image/jpeg",
                  "test411.jpg");
  CheckOneViolation("http://www.example.com/foo.jpeg", 10);
}

TEST_F(OptimizeImagesTest, BasicPng) {
  AddPngResource("http://www.example.com/foo.png",
                 "image/png",
                 "basi3p02.png");
  CheckOneViolation("http://www.example.com/foo.png", 80);
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

}  // namespace
