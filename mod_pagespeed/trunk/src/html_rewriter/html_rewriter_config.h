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

// Note: this file contains serveral helper function to get the configuaration
// of the instaweb rewriter driver.
// - where to the cache as files
// - what is the URL prefix for rewritten resources
// - what is the cache prefix for rewritren resource (cache prefix and the URL
//   prefix should point to the same resource)

#ifndef HTML_REWRITER_HTML_REWRITER_CONFIG_H_
#define HTML_REWRITER_HTML_REWRITER_CONFIG_H_

#include <string>

// Forward declaration.
struct request_rec;

namespace html_rewriter {

// Get the cache file prefix.
std::string GetCachePrefix(request_rec* request);
// Get the prefix of rewritten URLs.
std::string GetUrlPrefix(request_rec* request);
// Get the path name of file cache.
std::string GetFileCachePath(request_rec* request);
// Get the fetcher proxy
std::string GetFetcherProxy();

}  // namespace html_rewriter

#endif  // HTML_REWRITER_HTML_REWRITER_CONFIG_H_
