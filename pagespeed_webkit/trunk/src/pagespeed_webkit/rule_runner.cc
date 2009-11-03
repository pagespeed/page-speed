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

#include "pagespeed_webkit/rule_runner.h"

#include "InspectorController.h"
#include "InspectorResource.h"
#include "Page.h"
#include "PlatformString.h"
#include "SharedBuffer.h"

#include "pagespeed/core/engine.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/rule.h"
#include "pagespeed/formatters/text_formatter.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/rules/rule_provider.h"
#include "pagespeed_webkit/webkit_rule_provider.h"
#include "pagespeed_webkit/util.h"

namespace {

void PopulateInput(WebCore::Page* page, pagespeed::PagespeedInput* input) {
  typedef WebCore::InspectorController::ResourcesMap ResourcesMap;

  const ResourcesMap& resources = page->inspectorController()->resources();
  for (ResourcesMap::const_iterator it = resources.begin(),
           end = resources.end();
       it != end;
       ++it) {
    RefPtr<WebCore::InspectorResource> resource = it->second;
    if (!resource.get()) {
      continue;
    }

    pagespeed::Resource* pagespeed_resource = new pagespeed::Resource;

    pagespeed_resource->SetRequestUrl(
        pagespeed_webkit::ToAscii(resource->requestURL()));
    pagespeed_resource->SetRequestMethod(
        pagespeed_webkit::ToAscii(resource->requestMethod()));
    pagespeed_resource->SetRequestProtocol("HTTP/1.1");
    pagespeed_resource->SetResponseStatusCode(resource->responseStatusCode());
    pagespeed_resource->SetResponseProtocol("HTTP/1.1");

    WebCore::String textEncodingName;
    RefPtr<WebCore::SharedBuffer> buffer =
        resource->resourceData(&textEncodingName);
    if (buffer.get()) {
      std::string content(buffer->data(), buffer->size());
      pagespeed_resource->SetResponseBody(content);
    }

    for (WebCore::HTTPHeaderMap::const_iterator
             it = resource->requestHeaderFields().begin(),
             end = resource->requestHeaderFields().end();
         it != end;
         ++it) {
      pagespeed_resource->AddRequestHeader(
          pagespeed_webkit::ToAscii(it->first.string()),
          pagespeed_webkit::ToAscii(it->second));
    }

    for (WebCore::HTTPHeaderMap::const_iterator
             it = resource->responseHeaderFields().begin(),
             end = resource->responseHeaderFields().end();
         it != end;
         ++it) {
      pagespeed_resource->AddResponseHeader(
          pagespeed_webkit::ToAscii(it->first.string()),
          pagespeed_webkit::ToAscii(it->second));
    }

    input->AddResource(pagespeed_resource);
  }
}

}  // namespace

namespace pagespeed_webkit {

namespace rule_runner {

void ComputeAndFormatResults(WebCore::Page* page,
                             pagespeed::Formatter* formatter) {

  pagespeed::PagespeedInput input;
  PopulateInput(page, &input);

  std::vector<pagespeed::Rule*> rules;
  pagespeed::rule_provider::AppendCoreRules(&rules);
  pagespeed_webkit::rule_provider::AppendWebkitRules(page, &rules);

  pagespeed::Engine engine(rules);
  engine.Init();
  engine.ComputeAndFormatResults(input, formatter);
}

}  // namespace rule_runner

}  // namespace pagespeed_webkit
