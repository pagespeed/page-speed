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

#include "pagespeed/core/input_capabilities.h"
#include "testing/gtest/include/gtest/gtest.h"

using pagespeed::InputCapabilities;

namespace {

TEST(InputCapabilitiesTest, None) {
  InputCapabilities a;
  InputCapabilities b;
  ASSERT_EQ(0U, a.capabilities_mask());
  ASSERT_EQ(0U, b.capabilities_mask());
  ASSERT_TRUE(a.equals(b));
  ASSERT_TRUE(b.equals(a));
  ASSERT_TRUE(a.satisfies(b));
  ASSERT_TRUE(b.satisfies(a));
}

TEST(InputCapabilitiesTest, All) {
  InputCapabilities a(InputCapabilities::ALL);
  InputCapabilities b(InputCapabilities::ALL);
  ASSERT_EQ(kuint32max, a.capabilities_mask());
  ASSERT_EQ(kuint32max, b.capabilities_mask());
  ASSERT_TRUE(a.equals(b));
  ASSERT_TRUE(b.equals(a));
  ASSERT_TRUE(a.satisfies(b));
  ASSERT_TRUE(b.satisfies(a));
}

TEST(InputCapabilitiesTest, Different) {
  InputCapabilities a(InputCapabilities::DOM);
  InputCapabilities b(InputCapabilities::ONLOAD);
  ASSERT_FALSE(a.equals(b));
  ASSERT_FALSE(b.equals(a));
  ASSERT_FALSE(a.satisfies(b));
  ASSERT_FALSE(b.satisfies(a));
}

TEST(InputCapabilitiesTest, Subset) {
  InputCapabilities a(InputCapabilities::DOM);
  InputCapabilities b(InputCapabilities::DOM |
                      InputCapabilities::ONLOAD);
  ASSERT_FALSE(a.equals(b));
  ASSERT_FALSE(b.equals(a));
  ASSERT_FALSE(a.satisfies(b));
  ASSERT_TRUE(b.satisfies(a));
}

TEST(InputCapabilitiesTest, SizeOf) {
  // Since InputCapabilities is copied by value, we want to make sure
  // it remains a small type. This test is intended to catch anyone
  // adding additional members to InputCapabilities. Please try not to
  // add additional members to this class.
  ASSERT_EQ(4U, sizeof(InputCapabilities));
}

TEST(InputCapabilitiesTest, DebugString) {
  InputCapabilities a(InputCapabilities::DOM |
                      InputCapabilities::ONLOAD |
                      InputCapabilities::REQUEST_START_TIMES);
  ASSERT_EQ("(Has: DOM ONLOAD REQUEST_START_TIMES ** Lacks: "
            "REQUEST_HEADERS RESPONSE_BODY TIMELINE_DATA)",
            a.DebugString());
}

}  // namespace
