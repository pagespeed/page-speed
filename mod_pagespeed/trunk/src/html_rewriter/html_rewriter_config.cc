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

// This is temporary implementation for the configurations. This setting will
// not work for windows and/or many other platform.
// TODO(lsong): Use httpd.conf to configure the module.

#include "html_rewriter/html_rewriter_config.h"

#include "third_party/apache_httpd/include/httpd.h"
#include "third_party/apache_httpd/include/http_core.h"

namespace {

const char* kCacheDir = "/cache/";
const char* kCacheFilePrefix = "cache_pre_";
const char* kUrlPrefix = "http://localhost:9999/cache/cache_pre_";
const char* kFileCachePath = "/tmp/html_rewrite_cache";
const char* kFetcherProxy = "localhost:9999";

}  // namespace


namespace html_rewriter {

std::string GetCachePrefix(request_rec* request) {
  std::string cache_root = ap_document_root(request);
  cache_root.append(kCacheDir);
  std::string cache_prefix(cache_root);
  cache_prefix.append(kCacheFilePrefix);
  return cache_prefix;
}

std::string GetUrlPrefix(request_rec* request) {
  return kUrlPrefix;
}

std::string GetFileCachePath(request_rec* request) {
  return kFileCachePath;
}

std::string GetFetcherProxy() {
  return kFetcherProxy;
}

}  // namespace html_rewriter
