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

#include "base/scoped_ptr.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/browsing_context.h"
#include "pagespeed/core/resource_evaluation.h"
#include "pagespeed/core/resource_fetch.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/uri_util.h"
#include "pagespeed/testing/pagespeed_test.h"
#include "testing/gtest/include/gtest/gtest.h"

using pagespeed::BrowsingContext;
using pagespeed::BrowsingContextData;
using pagespeed::Resource;
using pagespeed::ResourceVector;
using pagespeed::ResourceEvaluation;
using pagespeed::ResourceFetch;
using pagespeed::TopLevelBrowsingContext;
using pagespeed::uri_util::GetResourceUrlFromActionUri;
using pagespeed::uri_util::UriType;

namespace {

static const char* kURL1 = "http://www.foo.com/";
static const char* kURL2 = "http://www.foo.com/script1.js";
static const char* kURL3 = "http://www.foo.com/frame1.html";
static const char* kURL4 = "http://www.foo.com/frame2.html";
static const char* kURL5 = "http://www.foo.com/frame3.html";

class BrowsingContextTest : public ::pagespeed_testing::PagespeedTest {};

void AssertUri(const std::string& uri, const std::string& expected_base_url,
               UriType expected_uri_type) {
  std::string base_url;
  UriType uri_type;
  ASSERT_TRUE(GetResourceUrlFromActionUri(uri, &base_url, &uri_type, NULL));
  ASSERT_EQ(expected_base_url, base_url);
  ASSERT_EQ(expected_uri_type, uri_type);
}

void AssertSingleResource(const BrowsingContext& context,
                          const Resource& resource) {
  ResourceVector resources;
  ASSERT_TRUE(context.GetResources(&resources));
  ASSERT_EQ(static_cast<size_t>(1), resources.size());
  ASSERT_EQ(&resource, resources.at(0));
}

TEST_F(BrowsingContextTest, SimpleContext) {
  Resource* main = NewResource(kURL1, 200);

  TopLevelBrowsingContext* context = NewTopLevelBrowsingContext(main);

  ASSERT_EQ(main, context->GetDocumentResourceOrNull());

  AssertUri(context->GetBrowsingContextUri(), kURL1,
            pagespeed::uri_util::BROWSING_CONTEXT);

  AssertSingleResource(*context, *main);

  Resource* script = NewResource(kURL2, 200);

  ResourceEvaluation* evaluation = context->AddResourceEvaluation(script);
  ASSERT_TRUE(evaluation != NULL);

  AssertUri(evaluation->GetResourceEvaluationUri(),
            kURL2, pagespeed::uri_util::EVAL);

  ASSERT_EQ(1, context->GetResourceEvaluationCount(*script));
  ASSERT_EQ(evaluation, &context->GetResourceEvaluation(*script, 0));

  ASSERT_EQ(evaluation, context->FindResourceEvaluation(
      evaluation->GetResourceEvaluationUri()));
}

TEST_F(BrowsingContextTest, NestedContextWithoutResourceAndEvaluation) {
  Resource* main = NewResource(kURL1, 200);

  TopLevelBrowsingContext* context = NewTopLevelBrowsingContext(main);

  ASSERT_EQ(main, context->GetDocumentResourceOrNull());

  BrowsingContext* context_1 = context->AddNestedBrowsingContext(NULL);
  AssertUri(context_1->GetBrowsingContextUri(), kURL1,
            pagespeed::uri_util::BROWSING_CONTEXT);

  BrowsingContext* context_2 = context->AddNestedBrowsingContext(NULL);
  AssertUri(context_2->GetBrowsingContextUri(),
            kURL1, pagespeed::uri_util::BROWSING_CONTEXT);

  ASSERT_EQ(2, context->GetNestedContextCount());
  ASSERT_EQ(context_1, &context->GetNestedContext(0));
  ASSERT_EQ(context_1, context->GetMutableNestedContext(0));
  ASSERT_EQ(context_2, &context->GetNestedContext(1));
  ASSERT_EQ(context_2, context->GetMutableNestedContext(1));

  ASSERT_EQ(0, context_1->GetNestedContextCount());

  ASSERT_EQ(context, context_1->GetParentContext());
  ASSERT_EQ(context, context_2->GetParentContext());

  BrowsingContext* context_1_1 = context_1->AddNestedBrowsingContext(NULL);
  AssertUri(context_1_1->GetBrowsingContextUri(), kURL1,
            pagespeed::uri_util::BROWSING_CONTEXT);

  ASSERT_EQ(context_1, context_1_1->GetParentContext());

  ASSERT_EQ(1, context_1->GetNestedContextCount());

  Resource* script = NewResource(kURL2, 200);

  ResourceEvaluation* evaluation = context_1_1->AddResourceEvaluation(script);
  ASSERT_TRUE(evaluation != NULL);

  ASSERT_EQ(0, context->GetResourceEvaluationCount(*script));
  ASSERT_EQ(0, context_1->GetResourceEvaluationCount(*script));
  ASSERT_EQ(0, context_2->GetResourceEvaluationCount(*script));
  ASSERT_EQ(1, context_1_1->GetResourceEvaluationCount(*script));
  ASSERT_EQ(evaluation, &context_1_1->GetResourceEvaluation(*script, 0));
  ASSERT_EQ(evaluation, context_1_1->GetMutableResourceEvaluation(*script, 0));

  ASSERT_EQ(evaluation, context->FindResourceEvaluation(
      evaluation->GetResourceEvaluationUri()));

  ResourceEvaluation* evaluation_2 = context_1->AddResourceEvaluation(
      script);
  ASSERT_TRUE(evaluation_2 != NULL);

  ASSERT_EQ(0, context->GetResourceEvaluationCount(*script));
  ASSERT_EQ(1, context_1->GetResourceEvaluationCount(*script));
  ASSERT_EQ(evaluation_2, &context_1->GetResourceEvaluation(*script, 0));
  ASSERT_EQ(evaluation_2, context_1->GetMutableResourceEvaluation(*script, 0));
  ASSERT_EQ(0, context_2->GetResourceEvaluationCount(*script));
  ASSERT_EQ(1, context_1_1->GetResourceEvaluationCount(*script));

  ASSERT_EQ(evaluation_2, context->FindResourceEvaluation(
      evaluation_2->GetResourceEvaluationUri()));

  ASSERT_EQ(context,
            context->FindBrowsingContext(context->GetBrowsingContextUri()));
  ASSERT_EQ(context_1,
            context->FindBrowsingContext(context_1->GetBrowsingContextUri()));
  ASSERT_EQ(context_2,
            context->FindBrowsingContext(context_2->GetBrowsingContextUri()));
  ASSERT_EQ(context_1_1,
            context->FindBrowsingContext(context_1_1->GetBrowsingContextUri()));
}

TEST_F(BrowsingContextTest, NestedContextWithResourceAndFetch) {
  Resource* main = NewResource(kURL1, 200);
  Resource* frame1 = NewResource(kURL3, 200);
  Resource* frame2 = NewResource(kURL4, 200);
  Resource* frame3 = NewResource(kURL5, 200);

  TopLevelBrowsingContext* context = NewTopLevelBrowsingContext(main);

  ASSERT_EQ(main, context->GetDocumentResourceOrNull());

  BrowsingContext* context_1 = context->AddNestedBrowsingContext(frame1);
  AssertUri(context_1->GetBrowsingContextUri(),
            kURL3, pagespeed::uri_util::BROWSING_CONTEXT);
  AssertSingleResource(*context_1, *frame1);

  BrowsingContext* context_2 = context->AddNestedBrowsingContext(frame2);
  AssertUri(context_2->GetBrowsingContextUri(),
            kURL4, pagespeed::uri_util::BROWSING_CONTEXT);
  AssertSingleResource(*context_2, *frame2);

  ASSERT_EQ(2, context->GetNestedContextCount());
  ASSERT_EQ(context_1, &context->GetNestedContext(0));
  ASSERT_EQ(context_1, context->GetMutableNestedContext(0));
  ASSERT_EQ(context_2, &context->GetNestedContext(1));
  ASSERT_EQ(context_2, context->GetMutableNestedContext(1));

  ASSERT_EQ(0, context_1->GetNestedContextCount());

  ASSERT_EQ(context, context_1->GetParentContext());
  ASSERT_EQ(context, context_2->GetParentContext());

  BrowsingContext* context_1_1 = context_1->AddNestedBrowsingContext(frame3);
  AssertUri(context_1_1->GetBrowsingContextUri(), kURL5,
            pagespeed::uri_util::BROWSING_CONTEXT);

  ASSERT_EQ(context_1, context_1_1->GetParentContext());

  ASSERT_EQ(1, context_1->GetNestedContextCount());

  Resource* script = NewResource(kURL2, 200);

  ResourceFetch* fetch = context_1_1->AddResourceFetch(script);
  ASSERT_TRUE(fetch != NULL);

  ASSERT_EQ(0, context->GetResourceFetchCount(*script));
  ASSERT_EQ(0, context_1->GetResourceFetchCount(*script));
  ASSERT_EQ(0, context_2->GetResourceFetchCount(*script));
  ASSERT_EQ(1, context_1_1->GetResourceFetchCount(*script));
  ASSERT_EQ(fetch, &context_1_1->GetResourceFetch(*script, 0));
  ASSERT_EQ(fetch, context_1_1->GetMutableResourceFetch(*script, 0));

  ASSERT_EQ(fetch, context->FindResourceFetch(fetch->GetResourceFetchUri()));

  ResourceFetch* fetch_2 = context_1->AddResourceFetch(script);
  ASSERT_TRUE(fetch_2 != NULL);

  ASSERT_EQ(0, context->GetResourceFetchCount(*script));
  ASSERT_EQ(1, context_1->GetResourceFetchCount(*script));
  ASSERT_EQ(fetch_2, &context_1->GetResourceFetch(*script, 0));
  ASSERT_EQ(fetch_2, context_1->GetMutableResourceFetch(*script, 0));
  ASSERT_EQ(0, context_2->GetResourceFetchCount(*script));
  ASSERT_EQ(1, context_1_1->GetResourceFetchCount(*script));

  ASSERT_EQ(fetch_2,
            context->FindResourceFetch(fetch_2->GetResourceFetchUri()));

  ASSERT_EQ(context,
            context->FindBrowsingContext(context->GetBrowsingContextUri()));
  ASSERT_EQ(context_1,
            context->FindBrowsingContext(context_1->GetBrowsingContextUri()));
  ASSERT_EQ(context_2,
            context->FindBrowsingContext(context_2->GetBrowsingContextUri()));
  ASSERT_EQ(context_1_1,
            context->FindBrowsingContext(context_1_1->GetBrowsingContextUri()));
}

TEST_F(BrowsingContextTest, FindUnknownContextFetchEval) {
  Resource* main = NewResource(kURL1, 200);

  TopLevelBrowsingContext* context = NewTopLevelBrowsingContext(main);

  ASSERT_TRUE(context->FindBrowsingContext("foo") == NULL);
  ASSERT_TRUE(context->FindResourceEvaluation("foo") == NULL);
  ASSERT_TRUE(context->FindResourceFetch("foo") == NULL);
}

TEST_F(BrowsingContextTest, FailUnknownResource) {
  Resource* main = NewResource(kURL1, 200);

  TopLevelBrowsingContext* context = NewTopLevelBrowsingContext(main);

  scoped_ptr<Resource> script(new Resource());
  script->SetRequestUrl(kURL2);
  script->SetResponseStatusCode(200);

  #ifdef NDEBUG
  ASSERT_EQ(NULL, context->AddResourceEvaluation(script.get()));
#else
  ASSERT_DEATH(context->AddResourceEvaluation(script.get()),
               "Cannot register child resource which is not added to the "
               "ResourceCollection.");
#endif
}

TEST_F(BrowsingContextTest, Serialize) {
  Resource* main = NewResource(kURL1, 200);
  Resource* frame1 = NewResource(kURL3, 200);
  Resource* script = NewResource(kURL2, 200);

  TopLevelBrowsingContext* context = NewTopLevelBrowsingContext(main);

  BrowsingContext* context_1 = context->AddNestedBrowsingContext(NULL);
  BrowsingContext* context_2 = context->AddNestedBrowsingContext(frame1);

  ResourceFetch* fetch = context->AddResourceFetch(script);
  ResourceEvaluation* eval = context->AddResourceEvaluation(script);

  ResourceFetch* fetch_1 = context_1->AddResourceFetch(script);
  ResourceEvaluation* eval_1 = context_1->AddResourceEvaluation(script);

  context->SetEventDomContentTiming(10, 100);
  context->SetEventLoadTiming(20, 200);

  BrowsingContextData data;
  ASSERT_TRUE(context->SerializeData(&data));

  ASSERT_EQ(context->GetBrowsingContextUri(), data.uri());
  ASSERT_EQ(kURL1, data.document_resource_url());
  ASSERT_EQ(2, data.resource_urls_size());

  ASSERT_EQ(1, data.fetch_size());
  ASSERT_EQ(fetch->GetResourceFetchUri(), data.fetch(0).uri());
  ASSERT_EQ(1, data.evaluation_size());
  ASSERT_EQ(eval->GetResourceEvaluationUri(), data.evaluation(0).uri());

  ASSERT_EQ(10, data.event_dom_content().tick());
  ASSERT_EQ(100, data.event_dom_content().msec());
  ASSERT_EQ(20, data.event_on_load().tick());
  ASSERT_EQ(200, data.event_on_load().msec());

  ASSERT_EQ(2, data.nested_context_size());
  ASSERT_EQ(context_1->GetBrowsingContextUri(), data.nested_context(0).uri());
  ASSERT_EQ(context_2->GetBrowsingContextUri(), data.nested_context(1).uri());

  ASSERT_EQ(context_1->GetBrowsingContextUri(), data.nested_context(0).uri());
  ASSERT_FALSE(data.nested_context(0).has_document_resource_url());
  ASSERT_EQ(1, data.nested_context(0).resource_urls_size());
  ASSERT_EQ(1, data.nested_context(0).fetch_size());
  ASSERT_EQ(fetch_1->GetResourceFetchUri(),
            data.nested_context(0).fetch(0).uri());
  ASSERT_EQ(1, data.nested_context(0).evaluation_size());
  ASSERT_EQ(eval_1->GetResourceEvaluationUri(),
            data.nested_context(0).evaluation(0).uri());

  ASSERT_EQ(context_2->GetBrowsingContextUri(), data.nested_context(1).uri());
  ASSERT_EQ(kURL3, data.nested_context(1).document_resource_url());
  ASSERT_EQ(1, data.nested_context(1).resource_urls_size());
  ASSERT_EQ(0, data.nested_context(1).fetch_size());
  ASSERT_EQ(0, data.nested_context(1).evaluation_size());
}

}  // namespace
