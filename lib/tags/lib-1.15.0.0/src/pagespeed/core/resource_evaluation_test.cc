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

#include <string>
#include <vector>

#include "base/memory/scoped_ptr.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/resource_fetch.h"
#include "pagespeed/core/resource_evaluation.h"
#include "pagespeed/core/browsing_context.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/uri_util.h"
#include "pagespeed/testing/pagespeed_test.h"
#include "testing/gtest/include/gtest/gtest.h"

using pagespeed::BrowsingContext;
using pagespeed::PagespeedInput;
using pagespeed::Resource;
using pagespeed::ResourceVector;
using pagespeed::EvaluationConstraintVector;
using pagespeed::ResourceEvaluation;
using pagespeed::ResourceFetch;
using pagespeed::ResourceEvaluationConstraint;
using pagespeed::ResourceEvaluationData;
using pagespeed::TopLevelBrowsingContext;
using pagespeed::uri_util::GetResourceUrlFromActionUri;
using pagespeed::uri_util::UriType;

namespace {

static const char* kURL1 = "http://www.foo.com/";
static const char* kURL2 = "http://www.foo.com/script1.js";

class ResourceEvaluationTest : public ::pagespeed_testing::PagespeedTest {};

void AssertUri(const std::string& uri, const std::string& expected_base_url,
               UriType expected_uri_type) {
  std::string base_url;
  UriType uri_type;
  ASSERT_TRUE(GetResourceUrlFromActionUri(uri, &base_url, &uri_type, NULL));
  ASSERT_EQ(expected_base_url, base_url);
  ASSERT_EQ(expected_uri_type, uri_type);
}

TEST_F(ResourceEvaluationTest, SimpleAndSerialization) {
  Resource* main = NewResource(kURL1, 200);
  Resource* script = NewResource(kURL2, 200);

  TopLevelBrowsingContext* context = NewTopLevelBrowsingContext(main);

  ResourceFetch* main_fetch = context->AddResourceFetch(main);
  ResourceEvaluation* main_eval = context->AddResourceEvaluation(main);
  AssertUri(main_eval->GetResourceEvaluationUri(),
            kURL1, pagespeed::uri_util::EVAL);
  main_eval->SetFetch(*main_fetch);

  ASSERT_EQ(main, &main_eval->GetResource());
  ASSERT_EQ(main_fetch, main_eval->GetFetch());

  ResourceFetch* script_fetch = context->AddResourceFetch(script);
  ResourceEvaluation* script_eval = context->AddResourceEvaluation(script);
  script_eval->SetFetch(*script_fetch);
  script_eval->SetEvaluationType(pagespeed::EVAL_SCRIPT);
  script_eval->SetTiming(10, 100, 20, 200);
  script_eval->SetIsAsync(true);
  script_eval->SetEvaluationLines(5, 7);

  ASSERT_EQ(script_fetch, script_eval->GetFetch());
  ASSERT_EQ(pagespeed::EVAL_SCRIPT, script_eval->GetEvaluationType());
  ASSERT_EQ(10, script_eval->GetStartTick());
  ASSERT_EQ(20, script_eval->GetFinishTick());
  ASSERT_FALSE(script_eval->IsMatchingMediaType());
  ASSERT_TRUE(script_eval->IsAsync());
  ASSERT_FALSE(script_eval->IsDefer());
  ASSERT_EQ(5, script_eval->GetEvaluationStartLine());
  ASSERT_EQ(7, script_eval->GetEvaluationEndLine());

  ResourceEvaluationConstraint* constraint_1 = script_eval->AddConstraint();
  constraint_1->SetConstraintType(pagespeed::BLOCKING);
  constraint_1->SetPredecessor(main_eval);

  ResourceEvaluationConstraint* constraint_2 = script_eval->AddConstraint();
  constraint_2->SetConstraintType(pagespeed::ASAP_ORDERED);

  ASSERT_EQ(2, script_eval->GetConstraintCount());
  ASSERT_EQ(constraint_1, &script_eval->GetConstraint(0));
  ASSERT_EQ(constraint_1, script_eval->GetMutableConstraint(0));
  ASSERT_EQ(constraint_2, &script_eval->GetConstraint(1));
  ASSERT_EQ(constraint_2, script_eval->GetMutableConstraint(1));

  EvaluationConstraintVector constraints;
  ASSERT_TRUE(script_eval->GetConstraints(&constraints));
  ASSERT_EQ(static_cast<size_t>(2), constraints.size());
  ASSERT_EQ(constraint_1, constraints.at(0));
  ASSERT_EQ(constraint_2, constraints.at(1));

  ASSERT_EQ(pagespeed::BLOCKING, constraint_1->GetConstraintType());
  ASSERT_EQ(main_eval, constraint_1->GetPredecessor());

  ASSERT_EQ(pagespeed::ASAP_ORDERED, constraint_2->GetConstraintType());
  ASSERT_EQ(NULL, constraint_2->GetPredecessor());

  ResourceEvaluationData data;

  ASSERT_TRUE(script_eval->SerializeData(&data));

  ASSERT_EQ(script_eval->GetResourceEvaluationUri(), data.uri());
  ASSERT_EQ(script->GetRequestUrl(), data.resource_url());
  ASSERT_EQ(script_fetch->GetResourceFetchUri(), data.fetch_uri());
  ASSERT_EQ(pagespeed::EVAL_SCRIPT, data.type());

  ASSERT_EQ(2, data.constraints_size());

  ASSERT_EQ(pagespeed::BLOCKING, data.constraints(0).type());
  ASSERT_EQ(main_eval->GetResourceEvaluationUri(),
            data.constraints(0).predecessor_uri());
  ASSERT_EQ(pagespeed::ASAP_ORDERED, data.constraints(1).type());
  ASSERT_FALSE(data.constraints(1).has_predecessor_uri());

  ASSERT_EQ(10, data.start().tick());
  ASSERT_EQ(100, data.start().msec());
  ASSERT_EQ(20, data.finish().tick());
  ASSERT_EQ(200, data.finish().msec());
  ASSERT_FALSE(data.is_matching_media_type());
  ASSERT_TRUE(data.is_async());
  ASSERT_FALSE(data.is_defer());
  ASSERT_EQ(5, data.block_start_line());
  ASSERT_EQ(7, data.block_end_line());
}

}  // namespace
