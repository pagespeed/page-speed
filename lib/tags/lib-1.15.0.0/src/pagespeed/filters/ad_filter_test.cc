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
#include "pagespeed/filters/ad_filter.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace pagespeed {

TEST(AdFilterTest, AdFilter) {
  Resource resource;
  AdFilter ad_filter;

  EXPECT_TRUE(ad_filter.IsAccepted(resource));

  resource.SetRequestUrl("http://www.google.com/");
  EXPECT_TRUE(ad_filter.IsAccepted(resource));

  resource.SetRequestUrl("http://ad.doubleclick.net/adj/etc");
  EXPECT_FALSE(ad_filter.IsAccepted(resource));

  resource.SetRequestUrl(
      "http://pagead2.googlesyndication.com/pagead/show_ads.js");
  EXPECT_FALSE(ad_filter.IsAccepted(resource));

  resource.SetRequestUrl(
      "http://partner.googleadservices.com/gampad/google_service.js");
  EXPECT_FALSE(ad_filter.IsAccepted(resource));

  resource.SetRequestUrl("http://x.azjmp.com/0nTZT?sub=mygirlyspace");
  EXPECT_FALSE(ad_filter.IsAccepted(resource));

  resource.SetRequestUrl("http://some.random.domain.com/ad.php");
  EXPECT_FALSE(ad_filter.IsAccepted(resource));

  resource.SetRequestUrl("http://wildcard.eert.net/bar");
  EXPECT_FALSE(ad_filter.IsAccepted(resource));
}

}  // namespace pagespeed
