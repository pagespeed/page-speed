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

#include <string>

#include "pagespeed/rules/sprite_images.h"
#include "pagespeed/testing/pagespeed_test.h"

namespace {

using pagespeed::rules::SpriteImages;
using pagespeed_testing::FakeDomDocument;
using pagespeed_testing::FakeDomElement;
using pagespeed_testing::FakeImageAttributesFactory;

class Violation {
 public:
  Violation(int _expected_rt_savings,
            const std::string& _host,
            const std::vector<std::string>& _urls)
      : expected_rt_savings(_expected_rt_savings),
        host(_host),
        urls(_urls) {
  }

  int expected_rt_savings;
  std::string host;
  std::vector<std::string> urls;
};


class SpriteImagesTest
    : public ::pagespeed_testing::PagespeedRuleTest<SpriteImages> {
 protected:
  static const char* kRootUrl;
  static const int kImgSizeBytes;

  virtual void DoSetUp() {
    NewPrimaryResource(kRootUrl);
    CreateHtmlHeadBodyElements();
  }

  pagespeed::Resource* CreatePngResource(
      const std::string& url, int size) {
    FakeDomElement* element;
    pagespeed::Resource* resource = NewPngResource(url, body(), &element);
    std::string response_body(size, 'x');
    resource->SetResponseBody(response_body);
    return resource;
  }

  void CheckFormattedOutput(const std::string& expected_output) {
    ASSERT_TRUE(AppendResults());
    EXPECT_EQ(expected_output, FormatResults());
  }

  void CheckExpectedViolations(const std::vector<Violation>& expected) {
    ASSERT_TRUE(AppendResults());
    ASSERT_EQ(expected.size(), static_cast<size_t>(num_results()));

    for (size_t idx = 0; idx < expected.size(); ++idx) {
      ASSERT_EQ(expected[idx].urls.size(),
                static_cast<size_t>(result(idx).resource_urls_size()));
      for (size_t jdx = 0; jdx < expected[idx].urls.size(); ++jdx) {
        EXPECT_EQ(expected[idx].urls[jdx], result(idx).resource_urls(jdx));
      }
    }
  }
};

const char* SpriteImagesTest::kRootUrl = "http://test.com/";
const int SpriteImagesTest::kImgSizeBytes = 50;

TEST_F(SpriteImagesTest, EmptyDom) {
  Freeze();
  std::vector<Violation> no_violations;
  CheckExpectedViolations(no_violations);
}

TEST_F(SpriteImagesTest, OneImage) {
  CreatePngResource("http://test.com/image.png", kImgSizeBytes);
  Freeze();
  std::vector<Violation> no_violations;
  CheckExpectedViolations(no_violations);

}

TEST_F(SpriteImagesTest, TwoImages) {
  const std::string url1 = "http://test.com/image1.png";
  const std::string url2 = "http://test.com/image2.png";
  pagespeed::Resource* resource1 = CreatePngResource(url1, kImgSizeBytes);
  pagespeed::Resource* resource2 = CreatePngResource(url2, kImgSizeBytes);
  FakeImageAttributesFactory::ResourceSizeMap size_map;
  size_map[resource1] = std::make_pair(42, 23);
  size_map[resource2] = std::make_pair(42, 23);
  AddFakeImageAttributesFactory(size_map);
  Freeze();
  std::vector<Violation> no_violations;
  CheckExpectedViolations(no_violations);
}

TEST_F(SpriteImagesTest, FiveImages) {
  const std::string url1 = "http://test.com/image1.png";
  const std::string url2 = "http://test.com/image2.png";
  const std::string url3 = "http://test.com/image3.png";
  const std::string url4 = "http://test.com/image4.png";
  const std::string url5 = "http://test.com/image5.png";
  pagespeed::Resource* resource1 = CreatePngResource(url1, kImgSizeBytes);
  pagespeed::Resource* resource2 = CreatePngResource(url2, kImgSizeBytes);
  pagespeed::Resource* resource3 = CreatePngResource(url3, kImgSizeBytes);
  pagespeed::Resource* resource4 = CreatePngResource(url4, kImgSizeBytes);
  pagespeed::Resource* resource5 = CreatePngResource(url5, kImgSizeBytes);
  FakeImageAttributesFactory::ResourceSizeMap size_map;
  size_map[resource1] = std::make_pair(42, 23);
  size_map[resource2] = std::make_pair(42, 23);
  size_map[resource3] = std::make_pair(42, 23);
  size_map[resource4] = std::make_pair(42, 23);
  size_map[resource5] = std::make_pair(42, 23);
  AddFakeImageAttributesFactory(size_map);
  Freeze();
  std::vector<std::string> urls;
  urls.push_back(url1);
  urls.push_back(url2);
  urls.push_back(url3);
  urls.push_back(url4);
  urls.push_back(url5);
  std::vector<Violation> violations;
  violations.push_back(Violation(1, "test.com", urls));
  CheckExpectedViolations(violations);
}


TEST_F(SpriteImagesTest, OneByOneImage) {
  FakeImageAttributesFactory::ResourceSizeMap size_map;
  const std::string url1 = "http://test.com/image1.png";
  const std::string url2 = "http://test.com/image2.png";
  const std::string url3 = "http://test.com/image3.png";
  const std::string url4 = "http://test.com/image4.png";
  const std::string url5 = "http://test.com/image5.png";
  pagespeed::Resource* resource1 = CreatePngResource(url1, kImgSizeBytes);
  size_map[resource1] = std::make_pair(1,1);
  pagespeed::Resource* resource2 = CreatePngResource(url2, kImgSizeBytes);
  size_map[resource2] = std::make_pair(42, 23);
  pagespeed::Resource* resource3 = CreatePngResource(url3, kImgSizeBytes);
  size_map[resource3] = std::make_pair(42, 23);
  pagespeed::Resource* resource4 = CreatePngResource(url4, kImgSizeBytes);
  pagespeed::Resource* resource5 = CreatePngResource(url5, kImgSizeBytes);
  size_map[resource4] = std::make_pair(42, 23);
  size_map[resource5] = std::make_pair(42, 23);
  AddFakeImageAttributesFactory(size_map);
  Freeze();
  std::vector<Violation> no_violations;
  CheckExpectedViolations(no_violations);
}

TEST_F(SpriteImagesTest, ThreeImagesAndTwo1x1) {
  FakeImageAttributesFactory::ResourceSizeMap size_map;
  const std::string url1 = "http://test.com/image1.png";
  const std::string url2 = "http://test.com/image2.png";
  const std::string url3 = "http://test.com/image3.png";
  const std::string url4 = "http://test.com/image4.png";
  const std::string url5 = "http://test.com/image5.png";
  pagespeed::Resource* resource1 = CreatePngResource(url1, kImgSizeBytes);
  size_map[resource1] = std::make_pair(42, 23);
  pagespeed::Resource* resource2 = CreatePngResource(url2, kImgSizeBytes);
  size_map[resource2] = std::make_pair(42, 23);
  pagespeed::Resource* resource3 = CreatePngResource(url3, kImgSizeBytes);
  size_map[resource3] = std::make_pair(42, 23);
  pagespeed::Resource* resource4 = CreatePngResource(url4, kImgSizeBytes);
  size_map[resource4] = std::make_pair(1,1);
  pagespeed::Resource* resource5 = CreatePngResource(url5, kImgSizeBytes);
  size_map[resource5] = std::make_pair(1,1);
  AddFakeImageAttributesFactory(size_map);
  Freeze();
  std::vector<Violation> no_violations;
  CheckExpectedViolations(no_violations);
}

TEST_F(SpriteImagesTest, BigImages) {
  FakeImageAttributesFactory::ResourceSizeMap size_map;
  const std::string url1 = "http://test.com/image1.png";
  const std::string url2 = "http://test.com/image2.png";
  const std::string url3 = "http://test.com/image3.png";
  const std::string url4 = "http://test.com/image4.png";
  const std::string url5 = "http://test.com/image5.png";
  const std::string url6 = "http://test.com/image6.png";
  const std::string url7 = "http://test.com/image7.png";
  pagespeed::Resource* resource1 = CreatePngResource(url1, kImgSizeBytes);
  size_map[resource1] = std::make_pair(42, 23);
  pagespeed::Resource* resource2 = CreatePngResource(url2, kImgSizeBytes);
  size_map[resource2] = std::make_pair(42, 23);
  pagespeed::Resource* resource3 = CreatePngResource(url3, kImgSizeBytes);
  size_map[resource3] = std::make_pair(42, 23);
  pagespeed::Resource* resource4 = CreatePngResource(url4, 100 * 1024);
  size_map[resource4] = std::make_pair(42, 23);
  pagespeed::Resource* resource5 = CreatePngResource(url5, kImgSizeBytes);
  size_map[resource5] = std::make_pair(42, 23);
  pagespeed::Resource* resource6 = CreatePngResource(url6, kImgSizeBytes);
  size_map[resource6] = std::make_pair(96, 96);
  pagespeed::Resource* resource7 = CreatePngResource(url7, kImgSizeBytes);
  size_map[resource7] = std::make_pair(96, 97);
  AddFakeImageAttributesFactory(size_map);
  Freeze();
  std::vector<std::string> urls;
  urls.push_back(url1);
  urls.push_back(url2);
  urls.push_back(url3);
  urls.push_back(url5);
  urls.push_back(url6);
  std::vector<Violation> violations;
  violations.push_back(Violation(1, "test.com", urls));
  CheckExpectedViolations(violations);
}

TEST_F(SpriteImagesTest, TwoDomains) {
  FakeImageAttributesFactory::ResourceSizeMap size_map;
  const std::string url1 = "http://test.com/image1.png";
  const std::string url2 = "http://test.com/image2.png";
  const std::string url3 = "http://test.com/image3.png";
  const std::string url4 = "http://test.com/image4.png";
  const std::string url5 = "http://test.com/image5.png";
  pagespeed::Resource* resource1 = CreatePngResource(url1, kImgSizeBytes);
  size_map[resource1] = std::make_pair(42, 23);
  pagespeed::Resource* resource2 = CreatePngResource(url2, kImgSizeBytes);
  size_map[resource2] = std::make_pair(42, 23);
  pagespeed::Resource* resource3 = CreatePngResource(url3, kImgSizeBytes);
  size_map[resource3] = std::make_pair(42, 23);
  pagespeed::Resource* resource4 = CreatePngResource(url4, kImgSizeBytes);
  size_map[resource4] = std::make_pair(42, 23);
  pagespeed::Resource* resource5 = CreatePngResource(url5, kImgSizeBytes);
  size_map[resource5] = std::make_pair(42, 23);

  const std::string url2_1 = "http://test2.com/image1.png";
  const std::string url2_2 = "http://test2.com/image2.png";
  const std::string url2_3 = "http://test2.com/image3.png";
  const std::string url2_4 = "http://test2.com/image4.png";
  const std::string url2_5 = "http://test2.com/image5.png";
  pagespeed::Resource* resource2_1 = CreatePngResource(url2_1, kImgSizeBytes);
  pagespeed::Resource* resource2_2 = CreatePngResource(url2_2, kImgSizeBytes);
  pagespeed::Resource* resource2_3 = CreatePngResource(url2_3, kImgSizeBytes);
  pagespeed::Resource* resource2_4 = CreatePngResource(url2_4, kImgSizeBytes);
  pagespeed::Resource* resource2_5 = CreatePngResource(url2_5, kImgSizeBytes);
  size_map[resource2_1] = std::make_pair(42, 23);
  size_map[resource2_2] = std::make_pair(42, 23);
  size_map[resource2_3] = std::make_pair(42, 23);
  size_map[resource2_4] = std::make_pair(42, 23);
  size_map[resource2_5] = std::make_pair(42, 23);
  AddFakeImageAttributesFactory(size_map);
  Freeze();
  std::vector<std::string> urls;
  urls.push_back(url1);
  urls.push_back(url2);
  urls.push_back(url3);
  urls.push_back(url4);
  urls.push_back(url5);
  std::vector<std::string> urls2;
  urls2.push_back(url2_1);
  urls2.push_back(url2_2);
  urls2.push_back(url2_3);
  urls2.push_back(url2_4);
  urls2.push_back(url2_5);
  std::vector<Violation> violations;
  violations.push_back(Violation(1, "test.com", urls));
  violations.push_back(Violation(1, "test2.com", urls2));
  CheckExpectedViolations(violations);
}

TEST_F(SpriteImagesTest, FormatTest) {
  std::string expected =
      "The following images served from test.com should be combined into as "
      "few images as possible using CSS sprites.\n"
      "  http://test.com/image1.png\n"
      "  http://test.com/image2.png\n"
      "  http://test.com/image3.png\n"
      "  http://test.com/image4.png\n"
      "  http://test.com/image5.png\n";

  FakeImageAttributesFactory::ResourceSizeMap size_map;
  const std::string url1 = "http://test.com/image1.png";
  const std::string url2 = "http://test.com/image2.png";
  const std::string url3 = "http://test.com/image3.png";
  const std::string url4 = "http://test.com/image4.png";
  const std::string url5 = "http://test.com/image5.png";
  pagespeed::Resource* resource1 = CreatePngResource(url1, kImgSizeBytes);
  size_map[resource1] = std::make_pair(42, 23);
  pagespeed::Resource* resource2 = CreatePngResource(url2, kImgSizeBytes);
  size_map[resource2] = std::make_pair(42, 23);
  pagespeed::Resource* resource3 = CreatePngResource(url3, kImgSizeBytes);
  size_map[resource3] = std::make_pair(42, 23);
  pagespeed::Resource* resource4 = CreatePngResource(url4, kImgSizeBytes);
  size_map[resource4] = std::make_pair(42, 23);
  pagespeed::Resource* resource5 = CreatePngResource(url5, kImgSizeBytes);
  size_map[resource5] = std::make_pair(42, 23);
  AddFakeImageAttributesFactory(size_map);
  Freeze();
  CheckFormattedOutput(expected);
}

TEST_F(SpriteImagesTest, FormatNoOutputTest) {
  Freeze();
  CheckFormattedOutput("");
}

}  // namespace
