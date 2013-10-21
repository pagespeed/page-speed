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

#include "pagespeed/proto/pagespeed_output_util.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

using pagespeed::Result;
using pagespeed::Results;
using pagespeed::RuleResults;
using pagespeed::proto::AllResultsHaveIds;
using pagespeed::proto::ClearResultIds;
using pagespeed::proto::PopulateResultIds;

TEST(PagespeedOutputUtilTest, Basic) {
  Results results;
  RuleResults* rule_results1 = results.add_rule_results();
  RuleResults* rule_results2 = results.add_rule_results();
  Result* result1 = rule_results1->add_results();
  Result* result2 = rule_results1->add_results();
  Result* result3 = rule_results2->add_results();
  Result* result4 = rule_results2->add_results();
  Result* result5 = rule_results2->add_results();
  ASSERT_FALSE(AllResultsHaveIds(results));
  ASSERT_FALSE(result1->has_id());
  ASSERT_FALSE(result2->has_id());
  ASSERT_FALSE(result3->has_id());
  ASSERT_FALSE(result4->has_id());
  ASSERT_FALSE(result5->has_id());

  ASSERT_TRUE(PopulateResultIds(&results));
  ASSERT_TRUE(AllResultsHaveIds(results));
  ASSERT_EQ(0, result1->id());
  ASSERT_EQ(1, result2->id());
  ASSERT_EQ(2, result3->id());
  ASSERT_EQ(3, result4->id());
  ASSERT_EQ(4, result5->id());

  // Populating IDs should fail if one or more IDs is already
  // assigned.
  ASSERT_FALSE(PopulateResultIds(&results));
  ASSERT_TRUE(AllResultsHaveIds(results));

  ClearResultIds(&results);
  ASSERT_FALSE(AllResultsHaveIds(results));
  ASSERT_FALSE(result1->has_id());
  ASSERT_FALSE(result2->has_id());
  ASSERT_FALSE(result3->has_id());
  ASSERT_FALSE(result4->has_id());
  ASSERT_FALSE(result5->has_id());

  // Make sure assignment fails when only some ids are assigned.
  result3->set_id(0);
  ASSERT_FALSE(PopulateResultIds(&results));
  ASSERT_FALSE(AllResultsHaveIds(results));
  ASSERT_FALSE(result1->has_id());
  ASSERT_FALSE(result2->has_id());
  ASSERT_TRUE(result3->has_id());
  ASSERT_FALSE(result4->has_id());
  ASSERT_FALSE(result5->has_id());
}

}  // namespace
