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

#include "pagespeed/core/resource_util.h"
#include "pagespeed/core/rule_input.h"
#include "pagespeed/testing/pagespeed_test.h"

using pagespeed::Resource;
using pagespeed::RuleInput;

class RuleInputTest : public ::pagespeed_testing::PagespeedTest {};

TEST_F(RuleInputTest, GetCompressedResponseBodySize) {
  Resource* r1 = NewScriptResource(kUrl1, NULL, NULL);
  std::string body(1000, 'a');
  r1->SetResponseBody(body);

  Freeze();

  RuleInput rule_input(*pagespeed_input());

  int compressed_size = 0;
  ASSERT_TRUE(
      rule_input.GetCompressedResponseBodySize(*r1, &compressed_size));

  // NOTE: this size can change if we change the gzip compression
  // implementation.
  ASSERT_EQ(29, compressed_size);

  // NOTE: we have no good way to verify that the rule input is
  // caching response body sizes other than to change the response
  // body of the resource and make sure that we continue to get the
  // old response body size, rather than a newly computed size. This
  // in itself is broken, since it should not be possible to mutate
  // Resource state once the PagespeedInput is frozen. If in the
  // future we support freezing of Resources, we will need to fix this
  // test as well. The best way to fix the test may be to provide a
  // compressor object (prehaps optional, and only overridden by
  // tests) to the RuleInput instance.
  r1->SetResponseBody("");
  int actual_compressed_size = 0;
  ASSERT_TRUE(pagespeed::resource_util::GetGzippedSize(
      r1->GetResponseBody(), &actual_compressed_size));

  // NOTE: this size can change if we change the gzip compression
  // implementation.
  ASSERT_EQ(20, actual_compressed_size);

  int cached_compressed_size = 0;
  ASSERT_TRUE(
      rule_input.GetCompressedResponseBodySize(*r1, &cached_compressed_size));

  // Here we verify that the cached compressed size is returned,
  // rather than the compressed size of the modified resposne body.
  ASSERT_NE(actual_compressed_size, cached_compressed_size);
  ASSERT_EQ(cached_compressed_size, compressed_size);
}
