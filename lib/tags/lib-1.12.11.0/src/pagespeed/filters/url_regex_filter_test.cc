// Copyright 2010 Google Inc. All Rights Reserved.
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

#include "pagespeed/core/resource.h"
#include "pagespeed/filters/url_regex_filter.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const char* kRegex = ".*www\\.example\\.com|.*foo\\.com/bar";

TEST(UrlRegexFilterTest, UrlRegexFilter) {
  pagespeed::Resource resource;
  pagespeed::UrlRegexFilter filter(kRegex);

  EXPECT_TRUE(filter.IsAccepted(resource));

  resource.SetRequestUrl("http://www.google.com/");
  EXPECT_TRUE(filter.IsAccepted(resource));

  resource.SetRequestUrl("http://www.example.com/");
  EXPECT_FALSE(filter.IsAccepted(resource));

  resource.SetRequestUrl("http://www.example.com/foobar");
  EXPECT_FALSE(filter.IsAccepted(resource));

  resource.SetRequestUrl("http://other.foo.com/bar");
  EXPECT_FALSE(filter.IsAccepted(resource));

  resource.SetRequestUrl("http://other.foo.com/");
  EXPECT_TRUE(filter.IsAccepted(resource));

  resource.SetRequestUrl("http://other.foo.com/foo");
  EXPECT_TRUE(filter.IsAccepted(resource));
}

}  // namespace
