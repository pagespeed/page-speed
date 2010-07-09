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
#include "base/basictypes.h"

// Forward declaration.
struct server_rec;

namespace html_rewriter {

// Get the cache file prefix.
std::string GetCachePrefix(server_rec* server);
// Get the prefix of rewritten URLs.
const char* GetUrlPrefix(server_rec* server);
// Get the path name of file cache.
const char* GetFileCachePath(server_rec* server);
// Get the fetcher proxy
const char* GetFetcherProxy(server_rec* server);
// Get the fetcher time out value in milliseconds.
int64 GetFetcherTimeOut(server_rec* server);
// Get the resource fetcher time out value in milliseconds.
// The resource may be fetched by a real client from the browser.
int64 GetResourceFetcherTimeOutMs(server_rec* server);

}  // namespace html_rewriter

#endif  // HTML_REWRITER_HTML_REWRITER_CONFIG_H_
