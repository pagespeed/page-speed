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

#include "pagespeed/filters/protocol_filter.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace pagespeed {

TEST(ProtocolFilterTest, ProtocolFilter) {
  Resource resource;
  std::vector<std::string> allowed_protocols;

  // No protocols allowed means everything is filtered
  ProtocolFilter empty_protocol_filter(&allowed_protocols);
  EXPECT_FALSE(empty_protocol_filter.IsAccepted(resource));

  // Allow http and https
  allowed_protocols.push_back("http");
  allowed_protocols.push_back("https");
  ProtocolFilter protocol_filter(&allowed_protocols);

  EXPECT_FALSE(protocol_filter.IsAccepted(resource));

  resource.SetRequestUrl("http://www.google.com/");
  EXPECT_TRUE(protocol_filter.IsAccepted(resource));

  resource.SetRequestUrl("https://gmail.com/");
  EXPECT_TRUE(protocol_filter.IsAccepted(resource));

  resource.SetRequestUrl("javascript:alert()");
  EXPECT_FALSE(protocol_filter.IsAccepted(resource));

  resource.SetRequestUrl("file:/usr/local/foo");
  EXPECT_FALSE(protocol_filter.IsAccepted(resource));

  // An https filter should reject http: and httpsuper:
  allowed_protocols.clear();
  allowed_protocols.push_back("https");
  ProtocolFilter https_filter(&allowed_protocols);

  resource.SetRequestUrl("http://www.google.com/");
  EXPECT_FALSE(https_filter.IsAccepted(resource));

  resource.SetRequestUrl("httpsuper://www.google.com/");
  EXPECT_FALSE(https_filter.IsAccepted(resource));
}

}  // namespace pagespeed
