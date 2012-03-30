// Copyright 2012 Google Inc. All Rights Reserved.
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
#include "pagespeed/core/browsing_context.h"
#include "pagespeed/browsing_context/browsing_context_factory.h"
#include "pagespeed/testing/pagespeed_test.h"

using pagespeed::browsing_context::BrowsingContextFactory;
using pagespeed_testing::FakeDomDocument;
using pagespeed_testing::FakeDomElement;

namespace pagespeed {

namespace {

class BrowsingContextFactoryTest : public ::pagespeed_testing::PagespeedTest {};

TEST_F(BrowsingContextFactoryTest, FindResources) {
  Resource* primary = NewPrimaryResource("http://www.example.com/");
  CreateHtmlHeadBodyElements();

  NewScriptResource("http://www.example.com/script.js", head(), NULL);

  FakeDomElement* iframe = FakeDomElement::NewIframe(body());
  FakeDomDocument* iframe_document = NULL;
  NewDocumentResource(
      "http://www.example.com/iframe.html", iframe, &iframe_document);

  BrowsingContextFactory context_factory(pagespeed_input());
  SetTopLevelBrowsingContext(
      context_factory.CreateTopLevelBrowsingContext(document(), primary));
  Freeze();

  const TopLevelBrowsingContext* top_level_context =
      pagespeed_input()->GetTopLevelBrowsingContext();

  ResourceVector top_level_resources;
  top_level_context->GetResources(&top_level_resources);
  ASSERT_EQ(static_cast<size_t>(3), top_level_resources.size());

  ASSERT_EQ(1, top_level_context->GetNestedContextCount());

  const BrowsingContext& nested = top_level_context->GetNestedContext(0);

  ResourceVector nested_resources;
  nested.GetResources(&nested_resources);
  ASSERT_EQ(static_cast<size_t>(1), nested_resources.size());
}

}  // namespace

}  // namespace pagespeed
