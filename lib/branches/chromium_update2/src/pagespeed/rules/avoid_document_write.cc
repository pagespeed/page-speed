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

#include "pagespeed/rules/avoid_document_write.h"

#include <algorithm>

#include "base/logging.h"
#include "base/string_util.h"
#include "net/instaweb/htmlparse/public/html_parse.h"
#include "net/instaweb/util/public/google_message_handler.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/javascript_call_info.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/resource_util.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/core/rule_input.h"
#include "pagespeed/html/external_resource_filter.h"
#include "pagespeed/l10n/l10n.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace pagespeed {

namespace {

// Gets an iterator in sibling_resources for the last resource found
// in external_resource_urls, or sibling_resources.end() if none of
// the resources in external_resource_urls could be found in
// sibling_resources.
ResourceVector::const_iterator FindLastExternalResourceInSiblingResources(
    const PagespeedInput& input,
    const std::vector<std::string>& external_resource_urls,
    const ResourceVector& sibling_resources) {
  // We want to find the last of the written resources in the
  // sibling_resources vector, so we iterate in reverse order.
  for (std::vector<std::string>::const_reverse_iterator it =
           external_resource_urls.rbegin(), end = external_resource_urls.rend();
       it != end;
       ++it) {
    const Resource* last_written_resource = input.GetResourceWithUrlOrNull(*it);
    if (last_written_resource == NULL) {
      LOG(INFO) << "Unable to find " << *it;
      continue;
    }
    if (last_written_resource->GetResourceType() == REDIRECT) {
      last_written_resource =
          resource_util::GetLastResourceInRedirectChain(
              input, *last_written_resource);
      if (last_written_resource == NULL) {
        LOG(INFO) << "Unable to find last redirected resource for " << *it;
        continue;
      }
    }

    // We found a resource. Now make sure that resource appears in the
    // sibling_resources vector.
    ResourceVector::const_iterator sib_it = find(sibling_resources.begin(),
                                                 sibling_resources.end(),
                                                 last_written_resource);
    if (sib_it != sibling_resources.end()) {
      return sib_it;
    }
  }

  // We failed to find any resources with a URL in
  // external_resource_urls.
  return sibling_resources.end();
}

bool DocumentContainsUserVisibleResource(const PagespeedInput& input,
                                         const Resource& resource);

bool IsUserVisibleResource(const PagespeedInput& input,
                           const Resource& resource) {
  // TODO: we would also flag if there is any text content after a
  // resource, since rendering of that text is blocked on this
  // fetch. That would require walking the DOM and having a DOM API to
  // get text nodes. For now we just look for resources after our
  // resource that were loaded before onload.
  switch (resource.GetResourceType()) {
    case IMAGE:
      return !pagespeed::resource_util::IsLikelyTrackingPixel(input, resource);
    case HTML:
      return DocumentContainsUserVisibleResource(input, resource);
    case TEXT:
    case FLASH:
      return true;
    default:
      return false;
  }
}

bool DocumentContainsUserVisibleResource(const PagespeedInput& input,
                                         const Resource& resource) {
  DCHECK(resource.GetResourceType() == HTML);
  const ParentChildResourceMap::const_iterator pcrm_it =
      input.GetParentChildResourceMap()->find(&resource);
  if (pcrm_it == input.GetParentChildResourceMap()->end()) {
    LOG(INFO) << "Failed to find " << resource.GetRequestUrl()
              << " in parent-child resource map.";
    return false;
  }
  for (ResourceVector::const_iterator it = pcrm_it->second.begin(),
           end = pcrm_it->second.end();
       it != end;
       ++it) {
    const Resource* child = *it;
    if (IsUserVisibleResource(input, *child)) {
      return true;
    }
  }
  return false;
}

// Does the given set of external resource URLs, written into the
// document via document.write(), block the renderer? They block the
// renderer if there is additional user-visible content that comes
// after them in the document (e.g. images, text, etc).
bool DoesBlockRender(const PagespeedInput& input,
                     const std::string& document_url,
                     const std::vector<std::string>& external_resource_urls) {
  const Resource* parent_resource =
      input.GetResourceWithUrlOrNull(document_url);
  if (parent_resource == NULL) {
    LOG(INFO) << "Unable to find document " << document_url;
    return false;
  }
  if (parent_resource->GetResourceType() == REDIRECT) {
    parent_resource = resource_util::GetLastResourceInRedirectChain(
        input, *parent_resource);
  }
  if (parent_resource == NULL) {
    LOG(INFO) << "Unable to find document " << document_url;
    return false;
  }
  DCHECK(parent_resource->GetResourceType() == HTML);

  const ParentChildResourceMap::const_iterator pcrm_it =
      input.GetParentChildResourceMap()->find(parent_resource);
  if (pcrm_it == input.GetParentChildResourceMap()->end()) {
    LOG(INFO) << "Unable to find parent-resource map entry for "
              << parent_resource->GetRequestUrl();
    return false;
  }

  // Attempt to find one of the resources that was document.written()
  // in the set of sibling resources.
  const ResourceVector& sibling_resources = pcrm_it->second;
  ResourceVector::const_iterator sib_it =
      FindLastExternalResourceInSiblingResources(
          input, external_resource_urls, sibling_resources);
  if (sib_it == sibling_resources.end()) {
    LOG(INFO) << "Unable to find any external resources among siblings.";
    return false;
  }

  // Advance past the last_written_resource to the next resource in
  // the set of siblings.
  ++sib_it;

  // Now iterate over the resources that were loaded after the
  // document.written() resource, looking for one that contains
  // user-visible content.
  for (ResourceVector::const_iterator sib_end = sibling_resources.end();
       sib_it != sib_end;
       ++sib_it) {
    const Resource* peer_resource = *sib_it;
    if (input.IsResourceLoadedAfterOnload(*peer_resource)) {
      // If the resource was loaded post-onload, we should not
      // consider it.
      continue;
    }
    if (IsUserVisibleResource(input, *peer_resource)) {
      return true;
    }
  }

  // We did not find any blocked resources.
  return false;
}

}  // namespace

namespace rules {

AvoidDocumentWrite::AvoidDocumentWrite()
    : pagespeed::Rule(pagespeed::InputCapabilities(
        pagespeed::InputCapabilities::DOM |
        pagespeed::InputCapabilities::JS_CALLS_DOCUMENT_WRITE |
        pagespeed::InputCapabilities::REQUEST_START_TIMES |
        pagespeed::InputCapabilities::ONLOAD |
        pagespeed::InputCapabilities::PARENT_CHILD_RESOURCE_MAP |
        pagespeed::InputCapabilities::JS_CALLS_DOCUMENT_WRITE)) {
}

const char* AvoidDocumentWrite::name() const {
  return "AvoidDocumentWrite";
}

UserFacingString AvoidDocumentWrite::header() const {
  // TRANSLATOR: The name of a Page Speed rule that tells webmasters to avoid
  // using the "document.write" command in their JavaScript code
  // ("document.write" is code, and should not be translated).  This appears in
  // a list of rule names generated by Page Speed, telling webmasters which
  // rules they broke in their website.
  return _("Avoid document.write");
}

bool AvoidDocumentWrite::AppendResults(const RuleInput& rule_input,
                                       ResultProvider* provider) {
  bool error = false;
  const PagespeedInput& input = rule_input.pagespeed_input();
  net_instaweb::GoogleMessageHandler message_handler;
  message_handler.set_min_message_type(net_instaweb::kError);
  net_instaweb::HtmlParse html_parse(&message_handler);
  html::ExternalResourceFilter filter(&html_parse);
  html_parse.AddFilter(&filter);

  for (int i = 0, num = input.num_resources(); i < num; ++i) {
    const Resource& resource = input.GetResource(i);
    if (input.IsResourceLoadedAfterOnload(resource)) {
      continue;
    }
    const std::vector<const JavaScriptCallInfo*>* calls =
        resource.GetJavaScriptCalls("document.write");
    if (calls == NULL || calls->size() == 0) {
      continue;
    }

    for (std::vector<const JavaScriptCallInfo*>::const_iterator it =
             calls->begin(), end = calls->end();
         it != end;
         ++it) {
      const JavaScriptCallInfo* call = *it;
      if (call->args().size() != 1) {
        LOG(DFATAL) << "Unexpected number of JS args.";
        error = true;
        continue;
      }

      const std::string& src = call->args()[0];
      html_parse.StartParse(resource.GetRequestUrl().c_str());
      html_parse.ParseText(src.data(), src.length());
      html_parse.FinishParse();

      std::vector<std::string> external_resource_urls;
      if (!filter.GetExternalResourceUrls(&external_resource_urls,
                                          input.dom_document(),
                                          call->document_url())) {
        continue;
      }

      if (!DoesBlockRender(
              input, call->document_url(), external_resource_urls)) {
        continue;
      }

      Result* result = provider->NewResult();
      result->add_resource_urls(resource.GetRequestUrl());

      // NOTE: In Firefox, document.write() of script tags serializes
      // fetches, at least through Firefox version 4, so the critical
      // path cost in Firefox can be higher.
      Savings* savings = result->mutable_savings();
      savings->set_critical_path_length_saved(1);

      ResultDetails* details = result->mutable_details();
      AvoidDocumentWriteDetails* adw_details =
          details->MutableExtension(
              AvoidDocumentWriteDetails::message_set_extension);

      adw_details->set_line_number(call->line_number());
      for (std::vector<std::string>::const_iterator
               it = external_resource_urls.begin(),
               end = external_resource_urls.end();
           it != end;
           ++it) {
        adw_details->add_urls(*it);
      }
    }
  }
  return !error;
}

void AvoidDocumentWrite::FormatResults(const ResultVector& results,
                                       RuleFormatter* formatter) {
  formatter->AddUrlBlock(
      // TRANSLATOR: This appears as a header before a list of URLs of resources
      // that use the JavaScript command "document.write" ("document.write" is
      // code, and should not be translated).  It describes how using
      // "document.write" can slow down your website (by forcing external
      // resources to load serially, not in parallel).
      _("Using document.write to fetch external resources can introduce "
        "serialization delays in the rendering of the page. The following "
        "resources use document.write to fetch external resources:"));
  for (ResultVector::const_iterator iter = results.begin(),
           end = results.end();
       iter != end;
       ++iter) {
    const Result& result = **iter;
    if (result.resource_urls_size() != 1) {
      LOG(DFATAL) << "Unexpected number of resource URLs.  Expected 1, Got "
                  << result.resource_urls_size() << ".";
      continue;
    }
    const ResultDetails& details = result.details();
    if (details.HasExtension(
            AvoidDocumentWriteDetails::message_set_extension)) {
      const AvoidDocumentWriteDetails& adw_details = details.GetExtension(
          AvoidDocumentWriteDetails::message_set_extension);
      if (adw_details.urls_size() > 0) {
        UrlBlockFormatter* body =
            formatter->AddUrlBlock(
                // TRANSLATOR: Describes a single resource that violates the
                // AvoidDocumentWrite rule by using the "document.write"
                // JavaScript command ("document.write" is code, and should not
                // be translated).  It gives the URL of the resource that uses
                // "document.write", and the line number of that
                // call.  Following this will be a list of the URLs that are
                // fetched as a result of that "document.write" call.  "$1" is a
                // format token that will be replaced with the URL of the
                // external resource that uses "document.write".  "$2" will be
                // replaced with the line number of the call to "document.write"
                // in that resource.
                _("$1 calls document.write on line $2 to fetch:"),
                UrlArgument(result.resource_urls(0)),
                IntArgument(adw_details.line_number()));
        for (int i = 0, size = adw_details.urls_size(); i < size; ++i) {
          body->AddUrl(adw_details.urls(i));
        }
      }
    }
  }
}

void AvoidDocumentWrite::SortResultsInPresentationOrder(
    ResultVector* rule_results) const {
  // AvoidDocumentWrite generates results in the order the violations
  // appear in the DOM, which is a reasonably good order. We could
  // improve it by placing violations for all resources that happen in
  // the main document above those that happen in iframes, but the
  // default order is good enough for now.
}


}  // namespace rules

}  // namespace pagespeed
