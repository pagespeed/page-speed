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

#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "testing/gtest/include/gtest/gtest.h"

using pagespeed::PagespeedInput;

namespace {

static const char* kURL1 = "http://www.foo.com/";
static const char* kURL2 = "http://www.bar.com/";

pagespeed::Resource* NewResource(const std::string& url, int status_code) {
  pagespeed::Resource* resource = new pagespeed::Resource;
  resource->SetRequestUrl(url);
  resource->SetResponseStatusCode(status_code);
  return resource;
}

TEST(PagespeedInputTest, DisallowDuplicates) {
  pagespeed::PagespeedInput input;

  EXPECT_TRUE(input.AddResource(NewResource(kURL1, 200)));
  EXPECT_TRUE(input.AddResource(NewResource(kURL2, 200)));
  ASSERT_EQ(input.num_resources(), 2);
  EXPECT_FALSE(input.AddResource(NewResource(kURL2, 200)));
  ASSERT_EQ(input.num_resources(), 2);
  EXPECT_EQ(input.GetResource(0).GetRequestUrl(), kURL1);
  EXPECT_EQ(input.GetResource(1).GetRequestUrl(), kURL2);
}

TEST(PagespeedInputTest, AllowDuplicates) {
  pagespeed::PagespeedInput input;
  input.set_allow_duplicate_resources();

  EXPECT_TRUE(input.AddResource(NewResource(kURL1, 200)));
  EXPECT_TRUE(input.AddResource(NewResource(kURL2, 200)));
  EXPECT_TRUE(input.AddResource(NewResource(kURL2, 200)));
  ASSERT_EQ(input.num_resources(), 3);
  EXPECT_EQ(input.GetResource(0).GetRequestUrl(), kURL1);
  EXPECT_EQ(input.GetResource(1).GetRequestUrl(), kURL2);
  EXPECT_EQ(input.GetResource(2).GetRequestUrl(), kURL2);
}

TEST(PagespeedInputTest, FilterBadResources) {
  pagespeed::PagespeedInput input;
  EXPECT_FALSE(input.AddResource(NewResource("", 0)));
  EXPECT_FALSE(input.AddResource(NewResource("", 200)));
  EXPECT_FALSE(input.AddResource(NewResource(kURL1, 0)));
  EXPECT_FALSE(input.AddResource(NewResource(kURL1, -1)));
}

TEST(PagespeedInputTest, FilterResources) {
  pagespeed::PagespeedInput input(
      new pagespeed::NotResourceFilter(new pagespeed::AllowAllResourceFilter));
  EXPECT_FALSE(input.AddResource(NewResource(kURL1, 200)));
}

}  // namespace
