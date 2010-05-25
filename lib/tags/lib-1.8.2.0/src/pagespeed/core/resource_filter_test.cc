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

#include "pagespeed/core/resource_filter.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace pagespeed {

TEST(ResourceFilterTest, AllowAllResourceFilter) {
  Resource resource;
  AllowAllResourceFilter allow_all_filter;

  EXPECT_TRUE(allow_all_filter.IsAccepted(resource));
}

TEST(ResourceFilterTest, NotResourceFilter) {
  Resource resource;
  AllowAllResourceFilter *allow_all_filter = new AllowAllResourceFilter();
  NotResourceFilter not_filter(allow_all_filter);

  EXPECT_FALSE(not_filter.IsAccepted(resource));

  // not not true = true
  AllowAllResourceFilter *allow_all_filter2 = new AllowAllResourceFilter();
  NotResourceFilter *not_filter2 = new NotResourceFilter(allow_all_filter2);
  NotResourceFilter not_not_filter(not_filter2);
  EXPECT_TRUE(not_not_filter.IsAccepted(resource));
}

TEST(ResourceFilterTest, AndResourceFilter) {
  Resource resource;

  // allow_all and allow_all = allow_all
  AllowAllResourceFilter *allow_all_filter1 = new AllowAllResourceFilter();
  AllowAllResourceFilter *allow_all_filter2 = new AllowAllResourceFilter();
  AndResourceFilter and_filter1(allow_all_filter1, allow_all_filter2);
  EXPECT_TRUE(and_filter1.IsAccepted(resource));

  // allow_all and not(allow_all) = allow nothing
  AllowAllResourceFilter *allow_all_filter3 = new AllowAllResourceFilter();
  NotResourceFilter *not_filter = new NotResourceFilter(allow_all_filter3);
  AllowAllResourceFilter *allow_all_filter4 = new AllowAllResourceFilter();
  AndResourceFilter and_filter2(allow_all_filter4, not_filter);
  EXPECT_FALSE(and_filter2.IsAccepted(resource));

  // not(allow_all) and not(allow_all) = allow nothing
  AllowAllResourceFilter *allow_all_filter5 = new AllowAllResourceFilter();
  NotResourceFilter *not_filter2 = new NotResourceFilter(allow_all_filter5);
  AllowAllResourceFilter *allow_all_filter6 = new AllowAllResourceFilter();
  NotResourceFilter *not_filter3 = new NotResourceFilter(allow_all_filter6);
  AndResourceFilter and_filter3(not_filter2, not_filter3);
  EXPECT_FALSE(and_filter3.IsAccepted(resource));
}

}  // namespace pagespeed
