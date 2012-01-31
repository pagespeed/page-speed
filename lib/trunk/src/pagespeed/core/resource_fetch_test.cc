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

#include "base/scoped_ptr.h"
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
using pagespeed::CodeLocation;
using pagespeed::ResourceVector;
using pagespeed::ResourceEvaluation;
using pagespeed::ResourceFetch;
using pagespeed::ResourceFetchDelay;
using pagespeed::ResourceFetchDownload;
using pagespeed::ResourceFetchData;
using pagespeed::TopLevelBrowsingContext;
using pagespeed::uri_util::GetResourceUrlFromActionUri;
using pagespeed::uri_util::UriType;

namespace {

static const char* kURL1 = "http://www.foo.com/";
static const char* kURL2 = "http://www.foo.com/script1.js";
static const char* kURL3 = "http://www.foo.com/script2.js";

class ResourceFetchTest : public ::pagespeed_testing::PagespeedTest {};

void AssertUri(const std::string& uri, const std::string& expected_base_url,
               UriType expected_uri_type) {
  std::string base_url;
  UriType uri_type;
  ASSERT_TRUE(GetResourceUrlFromActionUri(uri, &base_url, &uri_type, NULL));
  ASSERT_EQ(expected_base_url, base_url);
  ASSERT_EQ(expected_uri_type, uri_type);
}

void CreateFakeLocation(std::vector<CodeLocation*>* location_list) {
  CodeLocation* location = new CodeLocation();
  location->set_url(kURL1);
  location->set_line(10);
  location_list->push_back(location);

  location = new CodeLocation();
  location->set_url(kURL2);
  location->set_line(20);
  location_list->push_back(location);
}

TEST_F(ResourceFetchTest, Simple) {
  Resource* main = NewResource(kURL1, 200);
  Resource* script = NewResource(kURL2, 200);

  TopLevelBrowsingContext* context = NewTopLevelBrowsingContext(main);

  ResourceFetch* main_fetch = context->AddResourceFetch(main);
  AssertUri(main_fetch->GetResourceFetchUri(),
            kURL1, pagespeed::uri_util::FETCH);
  ResourceEvaluation* main_eval = context->AddResourceEvaluation(main);
  main_eval->SetFetch(*main_fetch);

  ASSERT_EQ(main, &main_fetch->GetResource());
  ASSERT_TRUE(main_fetch->GetRequestor() == NULL);

  ResourceFetch* script_fetch = context->AddResourceFetch(script);
  ASSERT_EQ(script, &script_fetch->GetResource());

  ASSERT_TRUE(script_fetch->GetMutableDownload()->SetRequestor(main_eval));
  script_fetch->SetDiscoveryType(pagespeed::DOCUMENT_WRITE);

  script_fetch->GetMutableDownload()->SetLoadTiming(10, 100, 20, 200);

  ASSERT_EQ(10, script_fetch->GetStartTick());
  ASSERT_EQ(20, script_fetch->GetFinishTick());

  std::vector<CodeLocation*> location;
  CreateFakeLocation(&location);
  script_fetch->AcquireCodeLocation(&location);
  ASSERT_TRUE(location.empty());

  ASSERT_EQ(2, script_fetch->GetCodeLocationCount());
  ASSERT_EQ(kURL1, script_fetch->GetCodeLocation(0).url());
  ASSERT_EQ(20, script_fetch->GetCodeLocation(1).line());

  ResourceFetchDelay* delay_timeout = script_fetch->AddFetchDelay();
  ASSERT_TRUE(delay_timeout != NULL);
  delay_timeout->SetTimeout(1000);
  ASSERT_EQ(pagespeed::TIMEOUT, delay_timeout->GetType());
  ASSERT_EQ(1000, delay_timeout->GetTimeoutMsec());

  CreateFakeLocation(&location);
  delay_timeout->AcquireCodeLocation(&location);
  ASSERT_TRUE(location.empty());

  ASSERT_EQ(2, delay_timeout->GetCodeLocationCount());
  ASSERT_EQ(kURL1, delay_timeout->GetCodeLocation(0).url());
  ASSERT_EQ(20, delay_timeout->GetCodeLocation(1).line());

  ResourceFetchDelay* delay_event = script_fetch->AddFetchDelay();
  ASSERT_TRUE(delay_event != NULL);
  delay_event->SetEvent("onload");
  ASSERT_EQ(pagespeed::EVENT, delay_event->GetType());
  ASSERT_EQ("onload", delay_event->GetEventName());

  ASSERT_EQ(2, script_fetch->GetFetchDelayCount());
  ASSERT_EQ(delay_timeout, &script_fetch->GetFetchDelay(0));
  ASSERT_EQ(delay_event, &script_fetch->GetFetchDelay(1));
}

TEST_F(ResourceFetchTest, Propagate) {
  Resource* main = NewResource(kURL1, 200);
  Resource* redirect = New302Resource(kURL2, kURL3);
  Resource* script = NewResource(kURL3, 200);

  TopLevelBrowsingContext* context = NewTopLevelBrowsingContext(main);

  ResourceFetch* main_fetch = context->AddResourceFetch(main);
  ResourceEvaluation* main_eval = context->AddResourceEvaluation(main);
  main_eval->SetFetch(*main_fetch);
  ResourceFetch* redirect_fetch = context->AddResourceFetch(redirect);
  ResourceEvaluation* redirect_eval = context->AddResourceEvaluation(
      redirect);
  redirect_eval->SetFetch(*redirect_fetch);
  redirect_fetch->GetMutableDownload()->SetRequestor(main_eval);
  redirect_fetch->SetDiscoveryType(pagespeed::DOCUMENT_WRITE);
  redirect_fetch->GetMutableDownload()->SetLoadTiming(10, 100, 20, 200);
  std::vector<CodeLocation*> location;
  CreateFakeLocation(&location);
  redirect_fetch->AcquireCodeLocation(&location);
  ResourceFetchDelay* delay_timeout = redirect_fetch->AddFetchDelay();
  delay_timeout->SetTimeout(1000);
  CreateFakeLocation(&location);
  delay_timeout->AcquireCodeLocation(&location);

  ResourceFetch* script_fetch = context->AddResourceFetch(script);
  script_fetch->GetMutableDownload()->SetRequestor(redirect_eval);
  script_fetch->GetMutableDownload()->SetLoadTiming(21, 200, 31, 300);

  const std::string& script_fetch_uri = script_fetch->GetResourceFetchUri();
  const std::string& redirect_fetch_uri = redirect_fetch->GetResourceFetchUri();

  ASSERT_EQ(1, context->GetResourceFetchCount(*script));
  ASSERT_TRUE(script_fetch->Finalize());

  ASSERT_EQ(script_fetch, context->FindResourceFetch(script_fetch_uri));
  ASSERT_EQ(redirect_fetch, context->FindResourceFetch(redirect_fetch_uri));

  ASSERT_EQ(1, context->GetResourceFetchCount(*script));
  ASSERT_EQ(script_fetch, &context->GetResourceFetch(*script, 0));
  ASSERT_EQ(pagespeed::DOCUMENT_WRITE, script_fetch->GetDiscoveryType());

  const ResourceFetchDownload& logical_download = script_fetch->GetDownload();
  ASSERT_EQ(main_eval, logical_download.GetRequestor());
  ASSERT_EQ(10, logical_download.GetStartTick());
  ASSERT_EQ(31, logical_download.GetFinishTick());

  ASSERT_EQ(main_eval, script_fetch->GetRequestor());
  ASSERT_EQ(10, script_fetch->GetStartTick());
  ASSERT_EQ(31, script_fetch->GetFinishTick());

  const ResourceFetchDownload* redirect_download =
      script_fetch->GetRedirectDownload();
  ASSERT_TRUE(redirect_download != NULL);

  ASSERT_EQ(redirect_eval, redirect_download->GetRequestor());
  ASSERT_EQ(21, redirect_download->GetStartTick());
  ASSERT_EQ(31, redirect_download->GetFinishTick());
}

TEST_F(ResourceFetchTest, Serialize) {
  Resource* main = NewResource(kURL1, 200);
  Resource* script = NewResource(kURL2, 200);

  TopLevelBrowsingContext* context = NewTopLevelBrowsingContext(main);

  ResourceFetch* main_fetch = context->AddResourceFetch(main);
  ResourceEvaluation* main_eval = context->AddResourceEvaluation(main);
  main_eval->SetFetch(*main_fetch);
  ResourceFetch* script_fetch = context->AddResourceFetch(script);
  script_fetch->GetMutableDownload()->SetRequestor(main_eval);
  script_fetch->SetDiscoveryType(pagespeed::DOCUMENT_WRITE);
  script_fetch->GetMutableDownload()->SetLoadTiming(10, 100, 20, 200);
  std::vector<CodeLocation*> location;
  CreateFakeLocation(&location);
  script_fetch->AcquireCodeLocation(&location);
  ResourceFetchDelay* delay_timeout = script_fetch->AddFetchDelay();
  delay_timeout->SetTimeout(1000);
  CreateFakeLocation(&location);
  delay_timeout->AcquireCodeLocation(&location);
  ResourceFetchDelay* delay_event = script_fetch->AddFetchDelay();
  delay_event->SetEvent("onload");

  ResourceFetchData data;

  ASSERT_TRUE(script_fetch->SerializeData(&data));

  ASSERT_EQ(script_fetch->GetResourceFetchUri(), data.uri());
  ASSERT_EQ(script->GetRequestUrl(), data.resource_url());
  ASSERT_EQ(pagespeed::DOCUMENT_WRITE, data.type());
  ASSERT_EQ(main_eval->GetResourceEvaluationUri(),
            data.download().requestor_uri());
  ASSERT_EQ(2, data.location_size());
  ASSERT_EQ(kURL1, data.location(0).url());
  ASSERT_EQ(20, data.location(1).line());

  ASSERT_EQ(2, data.delay_size());
  ASSERT_EQ(pagespeed::TIMEOUT, data.delay(0).type());
  ASSERT_EQ(1000, data.delay(0).timeout_msec());

  ASSERT_EQ(pagespeed::EVENT, data.delay(1).type());
  ASSERT_EQ("onload", data.delay(1).event_name());

  ASSERT_EQ(10, data.download().start().tick());
  ASSERT_EQ(100, data.download().start().msec());
  ASSERT_EQ(20, data.download().finish().tick());
  ASSERT_EQ(200, data.download().finish().msec());
}

}  // namespace
