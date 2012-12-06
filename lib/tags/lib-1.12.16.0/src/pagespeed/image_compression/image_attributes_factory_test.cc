/**
 * Copyright 2010 Google Inc.
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
#include <string>

#include "base/basictypes.h"
#include "base/scoped_ptr.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/image_compression/image_attributes_factory.h"
#include "pagespeed/testing/pagespeed_test.h"

using pagespeed::ImageAttributes;
using pagespeed::image_compression::ImageAttributesFactory;
using pagespeed::Resource;

namespace {

// The *_TEST_DIR_PATH macros are set by the gyp target that builds this file.
const char kGifTestDir[] = IMAGE_TEST_DIR_PATH "pngsuite/gif/";
const char kPngSuiteTestDir[] = IMAGE_TEST_DIR_PATH "pngsuite/";
const char kJpegTestDir[] = IMAGE_TEST_DIR_PATH "jpeg/";

void ReadImageToString(const std::string& dir,
                       const char* file_name,
                       std::string* dest) {
  std::string path = dir + file_name;
  ASSERT_TRUE(pagespeed_testing::ReadFileToString(path, dest));
}

class ImageAttributesFactoryTest : public ::testing::Test {
 protected:
  Resource* CreateJpegResource(const char* file_name) {
    std::string body;
    ReadImageToString(kJpegTestDir, file_name, &body);
    return CreateTestResource(file_name, "image/jpeg", body);
  }

  Resource* CreatePngResource(const char* file_name) {
    std::string body;
    ReadImageToString(kPngSuiteTestDir, file_name, &body);
    return CreateTestResource(file_name, "image/png", body);
  }

  Resource* CreateGifResource(const char* file_name) {
    std::string body;
    ReadImageToString(kGifTestDir, file_name, &body);
    return CreateTestResource(file_name, "image/gif", body);
  }

  Resource* CreateTestResource(const char* file_name,
                               const std::string &content_type,
                               const std::string &body) {
    std::string url = "http://www.example.com/";
    url += file_name;
    Resource* resource = new Resource;
    resource->SetRequestUrl(url);
    resource->SetRequestMethod("GET");
    resource->SetResponseStatusCode(200);
    resource->AddResponseHeader("Content-Type", content_type);
    resource->SetResponseBody(body);
    return resource;
  }
};

TEST_F(ImageAttributesFactoryTest, ValidPng) {
  ImageAttributesFactory factory;
  scoped_ptr<Resource> resource(CreatePngResource("basi0g01.png"));
  ASSERT_LT(static_cast<size_t>(0), resource->GetResponseBody().size());
  scoped_ptr<ImageAttributes> image_attributes(
      factory.NewImageAttributes(resource.get()));
  ASSERT_NE(static_cast<ImageAttributes*>(NULL), image_attributes.get());
  EXPECT_EQ(32, image_attributes->GetImageWidth());
  EXPECT_EQ(32, image_attributes->GetImageHeight());
}

TEST_F(ImageAttributesFactoryTest, InvalidPng) {
  ImageAttributesFactory factory;
  scoped_ptr<Resource> resource(CreatePngResource("xcrn0g04.png"));
  ASSERT_LT(static_cast<size_t>(0), resource->GetResponseBody().size());
  scoped_ptr<ImageAttributes> image_attributes(
      factory.NewImageAttributes(resource.get()));
  ASSERT_EQ(static_cast<ImageAttributes*>(NULL), image_attributes.get());
}

TEST_F(ImageAttributesFactoryTest, ValidGif) {
  ImageAttributesFactory factory;
  scoped_ptr<Resource> resource(CreateGifResource("basi0g01.gif"));
  ASSERT_LT(static_cast<size_t>(0), resource->GetResponseBody().size());
  scoped_ptr<ImageAttributes> image_attributes(
      factory.NewImageAttributes(resource.get()));
  ASSERT_NE(static_cast<ImageAttributes*>(NULL), image_attributes.get());
  EXPECT_EQ(32, image_attributes->GetImageWidth());
  EXPECT_EQ(32, image_attributes->GetImageHeight());
}

TEST_F(ImageAttributesFactoryTest, ValidJpeg) {
  ImageAttributesFactory factory;
  scoped_ptr<Resource> resource(CreateJpegResource("sjpeg1.jpg"));
  ASSERT_LT(static_cast<size_t>(0), resource->GetResponseBody().size());
  scoped_ptr<ImageAttributes> image_attributes(
      factory.NewImageAttributes(resource.get()));
  ASSERT_NE(static_cast<ImageAttributes*>(NULL), image_attributes.get());
  EXPECT_EQ(120, image_attributes->GetImageWidth());
  EXPECT_EQ(90, image_attributes->GetImageHeight());
}

TEST_F(ImageAttributesFactoryTest, InvalidJpeg) {
  ImageAttributesFactory factory;
  scoped_ptr<Resource> resource(CreateJpegResource("notajpeg.png"));
  ASSERT_LT(static_cast<size_t>(0), resource->GetResponseBody().size());
  scoped_ptr<ImageAttributes> image_attributes(
      factory.NewImageAttributes(resource.get()));
  ASSERT_EQ(static_cast<ImageAttributes*>(NULL), image_attributes.get());
}

}  // namespace
