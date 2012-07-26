// Copyright 2011 Google Inc. All Rights Reserved.
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

#include "pagespeed/core/dom.h"
#include "pagespeed/testing/pagespeed_test.h"

namespace {

TEST(DomRectTest, Empty) {
  {
    pagespeed::DomRect r(0, 0, 0, 0);
    ASSERT_TRUE(r.IsEmpty());
  }
  {
    pagespeed::DomRect r(0, 0, -1, 0);
    ASSERT_TRUE(r.IsEmpty());
  }
  {
    pagespeed::DomRect r(0, 0, 0, -1);
    ASSERT_TRUE(r.IsEmpty());
  }
  {
    pagespeed::DomRect r(0, 0, 1, 0);
    ASSERT_TRUE(r.IsEmpty());
  }
  {
    pagespeed::DomRect r(0, 0, 0, 1);
    ASSERT_TRUE(r.IsEmpty());
  }
  {
    pagespeed::DomRect r(0, 0, 1, 1);
    ASSERT_FALSE(r.IsEmpty());
  }
}

TEST(DomRectTest, Intersection1) {
  pagespeed::DomRect r1(2, 4, 6, 8);
  pagespeed::DomRect r2(4, 6, 8, 10);

  pagespeed::DomRect intersection = r1.Intersection(r2);
  ASSERT_EQ(4, intersection.x());
  ASSERT_EQ(6, intersection.y());
  ASSERT_EQ(4, intersection.width());
  ASSERT_EQ(6, intersection.height());
  ASSERT_FALSE(intersection.IsEmpty());
}

TEST(DomRectTest, Intersection2) {
  pagespeed::DomRect r1(2, 4, 6, 8);
  pagespeed::DomRect r2(10, 12, 14, 16);

  pagespeed::DomRect intersection = r1.Intersection(r2);
  ASSERT_EQ(10, intersection.x());
  ASSERT_EQ(12, intersection.y());
  ASSERT_EQ(0, intersection.width());
  ASSERT_EQ(0, intersection.height());
  ASSERT_TRUE(intersection.IsEmpty());
}

TEST(DomRectTest, Intersection3) {
  pagespeed::DomRect r1(0, 0, 1024, 768);
  pagespeed::DomRect r2(100, 100, 200, 200);

  pagespeed::DomRect intersection = r1.Intersection(r2);
  ASSERT_EQ(100, intersection.x());
  ASSERT_EQ(100, intersection.y());
  ASSERT_EQ(200, intersection.width());
  ASSERT_EQ(200, intersection.height());
  ASSERT_FALSE(intersection.IsEmpty());
}

}  // namespace
