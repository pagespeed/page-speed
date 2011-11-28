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

#include "pagespeed/core/uri_util.h"

#include <string>
#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "googleurl/src/gurl.h"
#include "googleurl/src/url_canon.h"
#include "pagespeed/core/dom.h"
#include "third_party/domain_registry_provider/src/domain_registry/domain_registry.h"

namespace {

class DocumentFinderVisitor : public pagespeed::DomElementVisitor {
 public:
  explicit DocumentFinderVisitor(const std::string& url)
      : url_(url), document_(NULL) {}

  virtual void Visit(const pagespeed::DomElement& node);

  bool HasDocument() { return document_.get() != NULL; }
  pagespeed::DomDocument* AcquireDocument() {
    DCHECK(HasDocument());
    return document_.release();
  }

 private:
  const std::string& url_;
  scoped_ptr<pagespeed::DomDocument> document_;

  DISALLOW_COPY_AND_ASSIGN(DocumentFinderVisitor);
};

void DocumentFinderVisitor::Visit(const pagespeed::DomElement& node) {
  if (HasDocument()) {
    // Already found a document so we do not need to visit any
    // additional nodes.
    return;
  }

  if (node.GetTagName() != "IFRAME") {
    return;
  }

  scoped_ptr<pagespeed::DomDocument> child_doc(node.GetContentDocument());
  if (child_doc.get() == NULL) {
    // Failed to get the child document, so bail.
    return;
  }

  // TODO: consider performing a match after removing the document
  // fragments.
  if (child_doc->GetDocumentUrl() == url_) {
    // We found the document instance, so hold onto it.
    document_.reset(child_doc.release());
    return;
  }

  // Search for the document within this child document.
  DocumentFinderVisitor visitor(url_);
  child_doc->Traverse(&visitor);
  if (visitor.HasDocument()) {
    // We found a matching document.
    document_.reset(visitor.AcquireDocument());
  }
}

GURL GetUriWithoutFragmentInternal(const GURL& url) {
  DCHECK(url.is_valid());
  url_canon::Replacements<char> clear_fragment;
  clear_fragment.ClearRef();
  return url.ReplaceComponents(clear_fragment);
}

// Code based on Chromium's
// RegistryControlledDomainService::GetDomainAndRegistryImpl.
std::string GetDomainAndRegistryImpl(const std::string& host) {
  DCHECK(!host.empty());

  // Skip leading dots.
  const size_t host_check_begin = host.find_first_not_of('.');
  if (host_check_begin == std::string::npos)
    return std::string();  // Host is only dots.
  const size_t trimmed_host_len = host.length() - host_check_begin;

  // Find the length of the registry for this host.
  const size_t registry_length =
      GetRegistryLengthAllowUnknownRegistries(host.c_str());
  if (registry_length == 0 || registry_length >= trimmed_host_len)
    return std::string();  // No registry.
  // The "2" in this next line is 1 for the dot, plus a 1-char minimum preceding
  // subcomponent length.
  DCHECK(host.length() >= 2);
  if (registry_length > (host.length() - 2)) {
    LOG(DFATAL) <<
        "Host does not have at least one subcomponent before registry!";
    return std::string();
  }

  // Move past the dot preceding the registry, and search for the next previous
  // dot.  Return the host from after that dot, or the whole host when there is
  // no dot.
  const size_t registry_dot = host.length() - (registry_length + 1);
  const size_t start = host.rfind('.', registry_dot - 1);
  if (start == std::string::npos)
    return host;
  return host.substr(start + 1);
}

}  // namespace

namespace pagespeed {

namespace uri_util {

void CanonicalizeUrl(std::string* inout_url) {
  GURL url(*inout_url);
  if (!url.is_valid()) {
    return;
  }
  *inout_url = url.spec();
}

bool GetUriWithoutFragment(const std::string& uri, std::string* out) {
  GURL url(uri);
  if (!url.is_valid()) {
    return false;
  }
  GURL url_no_fragment = GetUriWithoutFragmentInternal(url);
  if (!url_no_fragment.is_valid()) {
    // Should never happen.
    DCHECK(false);
    return false;
  }
  *out = url_no_fragment.spec();
  return true;
}

std::string ResolveUri(const std::string& uri, const std::string& base_url) {
  GURL url(base_url);
  if (!url.is_valid()) {
    return "";
  }

  GURL derived = url.Resolve(uri);
  if (!derived.is_valid()) {
    return "";
  }

  // Remove everything after the #, which is not sent to the server,
  // and return the resulting url.
  //
  // TODO: this should probably not be the default behavior; user
  // should have to explicitly remove the fragment.
  GURL url_no_fragment = GetUriWithoutFragmentInternal(derived);
  if (!url_no_fragment.is_valid()) {
    // Should never happen.
    DCHECK(false);
    return "";
  }
  return url_no_fragment.spec();
}

bool ResolveUriForDocumentWithUrl(
    const std::string& uri_to_resolve,
    const pagespeed::DomDocument* root_document,
    const std::string& document_url_to_find,
    std::string* out_resolved_url) {
  if (root_document == NULL) {
    LOG(INFO) << "No document. Unable to ResolveUriForDocumentWithUrl.";
    return false;
  }

  if (root_document->GetDocumentUrl() == document_url_to_find) {
    *out_resolved_url = root_document->ResolveUri(uri_to_resolve);
    return true;
  }

  DocumentFinderVisitor visitor(document_url_to_find);
  root_document->Traverse(&visitor);
  if (!visitor.HasDocument()) {
    return false;
  }

  scoped_ptr<pagespeed::DomDocument> doc(visitor.AcquireDocument());
  *out_resolved_url = doc->ResolveUri(uri_to_resolve);
  return true;
}

bool IsExternalResourceUrl(const std::string& url) {
  GURL gurl(url);
  if (!gurl.is_valid()) {
    return false;
  }

  return !gurl.SchemeIs("data");
}

// Code based on Chromium's
// RegistryControlledDomainService::GetDomainAndRegistry.
std::string GetDomainAndRegistry(const std::string& url) {
  GURL gurl(url);
  const url_parse::Component host =
      gurl.parsed_for_possibly_invalid_spec().host;
  if ((host.len <= 0) || gurl.HostIsIPAddress())
    return std::string();
  return GetDomainAndRegistryImpl(std::string(
      gurl.possibly_invalid_spec().data() + host.begin, host.len));
}

}  // namespace uri_util

}  // namespace pagespeed
