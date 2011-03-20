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

#ifndef PAGESPEED_CORE_URI_UTIL_H_
#define PAGESPEED_CORE_URI_UTIL_H_

#include <string>

namespace pagespeed {

class DomDocument;

namespace uri_util {

// Get the given URI, and remove its fragment if it has one. For
// instance, http://www.example.com/foo#fragment will return
// http://www.example.com/foo while http://www.example.com/bar will
// return http://www.example.com/bar.
bool GetUriWithoutFragment(const std::string& uri, std::string* out);

// Canonicalize the given URL. For instance, http://www.foo.com will
// become http://www.foo.com/.
void CanonicalizeUrl(std::string* inout_url);

// Resolve the specified URI relative to the given base URL.
std::string ResolveUri(const std::string& uri, const std::string& base_url);

// Attempt to resolve the specified URI relative to the document with
// the given URL. This method will search through all of the child
// documents of the specified root DomDocument, looking for a
// DomDocument with the specified document_url. If such a DomDocument
// is found, the specified URI will be resolved relative to that
// document and this method will return true. Otherwise this method
// will return false. Upon returning false, callers may choose to fall
// back to calling ResolveUri(), which will generate the correct
// result except in cases where the DomDocument contains a <base> tag
// that overrides its base URL.
bool ResolveUriForDocumentWithUrl(
    const std::string& uri_to_resolve,
    const pagespeed::DomDocument* root_document,
    const std::string& document_url_to_find,
    std::string* out_resolved_url);

// Is the given URL for an external resource, as opposed to an inline
// resource like a data URI?
bool IsExternalResourceUrl(const std::string& uri);

}  // namespace uri_util

}  // namespace pagespeed

#endif  // PAGESPEED_CORE_URI_UTIL_H_
