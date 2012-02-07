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

#include "pagespeed/browsing_context/browsing_context_factory.h"

#include <set>
#include <string>
#include <vector>

#include "base/scoped_ptr.h"
#include "net/instaweb/htmlparse/public/html_parse.h"
#include "net/instaweb/util/public/google_message_handler.h"
#include "net/instaweb/util/public/message_handler.h"
#include "pagespeed/core/browsing_context.h"
#include "pagespeed/core/dom.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/css/external_resource_finder.h"
#include "pagespeed/html/external_resource_filter.h"

namespace pagespeed {

namespace {

class BrowsingContextDomResourceVisitor
    : public ExternalResourceDomElementVisitor {
 public:
  explicit BrowsingContextDomResourceVisitor(
      const PagespeedInput* pagespeed_input);
  virtual ~BrowsingContextDomResourceVisitor();

  TopLevelBrowsingContext* CreateTopLevelBrowsingContext(
      const DomDocument* document,
      const Resource* primary_resource);

  virtual void VisitUrl(const DomElement& node,
                        const std::string& url);

  virtual void VisitDocument(const DomElement& element,
                             const DomDocument& document);

 private:
  const PagespeedInput* const pagespeed_input_;
  BrowsingContext* current_context_;
};

BrowsingContextDomResourceVisitor::BrowsingContextDomResourceVisitor(
    const PagespeedInput* const pagespeed_input)
    : pagespeed_input_(pagespeed_input) {}

BrowsingContextDomResourceVisitor::~BrowsingContextDomResourceVisitor() {}

TopLevelBrowsingContext*
BrowsingContextDomResourceVisitor::CreateTopLevelBrowsingContext(
    const DomDocument* document,
    const Resource* primary_resource) {
  scoped_ptr<TopLevelBrowsingContext> top_level_context(
      new TopLevelBrowsingContext(primary_resource, pagespeed_input_));
  current_context_ = top_level_context.get();
  top_level_context->AcquireDomDocument(document->Clone());

  scoped_ptr<DomElementVisitor> visitor(
      MakeDomElementVisitorForDocument(document, this));
  document->Traverse(visitor.get());
  return top_level_context.release();
}

void BrowsingContextDomResourceVisitor::VisitUrl(const DomElement& node,
                                                 const std::string& url) {
  const Resource* resource = pagespeed_input_->GetResourceWithUrlOrNull(url);
  if (resource != NULL) {
    current_context_->RegisterResource(resource);

    if (resource->GetResourceType() == pagespeed::CSS) {
      pagespeed::css::ExternalResourceFinder css_resource_finder;
      std::set<std::string> resource_urls;
      css_resource_finder.FindExternalResources(*resource, &resource_urls);
      for (std::set<std::string>::const_iterator it = resource_urls.begin();
          it != resource_urls.end(); ++it) {
        const Resource* resource = pagespeed_input_->GetResourceWithUrlOrNull(
            *it);
        if (resource != NULL) {
          current_context_->RegisterResource(resource);
        }
      }
    } else if (resource->GetResourceType() == pagespeed::HTML) {
      net_instaweb::GoogleMessageHandler message_handler;
      message_handler.set_min_message_type(net_instaweb::kError);

      net_instaweb::HtmlParse html_parse(&message_handler);
      html::ExternalResourceFilter html_resource_filter(&html_parse);
      html_parse.AddFilter(&html_resource_filter);
      html_parse.StartParse(resource->GetRequestUrl());
      html_parse.ParseText(resource->GetResponseBody().data(),
                           resource->GetResponseBody().length());
      html_parse.FinishParse();

      std::vector<std::string> url_list;
      html_resource_filter.GetExternalResourceUrls(
          &url_list, current_context_->GetDomDocument(),
          resource->GetRequestUrl());
      for (std::vector<std::string>::const_iterator it = url_list.begin();
          it != url_list.end(); ++it) {
        const Resource* resource =
            pagespeed_input_->GetResourceWithUrlOrNull(*it);
        if (resource != NULL) {
          current_context_->RegisterResource(resource);
        }
      }
    }
  }
}

void BrowsingContextDomResourceVisitor::VisitDocument(
    const DomElement& element, const DomDocument& document) {
  std::string document_url = document.GetDocumentUrl();
  const Resource* document_resource = NULL;
  if (!document_url.empty()) {
    document_resource = pagespeed_input_->GetResourceWithUrlOrNull(
        document_url);
  }
  BrowsingContext* parent_context = current_context_;
  current_context_ =
      parent_context->AddNestedBrowsingContext(document_resource);
  current_context_->AcquireDomDocument(document.Clone());

  scoped_ptr<DomElementVisitor> visitor(
      MakeDomElementVisitorForDocument(&document, this));
  document.Traverse(visitor.get());

  current_context_ = parent_context;
}

}  // namespace

namespace browsing_context {

BrowsingContextFactory::BrowsingContextFactory(
    const PagespeedInput* pagespeed_input) : pagespeed_input_(pagespeed_input) {
}

TopLevelBrowsingContext* BrowsingContextFactory::CreateTopLevelBrowsingContext(
    const DomDocument* document, const Resource* primary_resource) {
  BrowsingContextDomResourceVisitor visitor(pagespeed_input_);
  return visitor.CreateTopLevelBrowsingContext(document, primary_resource);
}

}  // namespace browsing_context
}  // namespace pagespeed
