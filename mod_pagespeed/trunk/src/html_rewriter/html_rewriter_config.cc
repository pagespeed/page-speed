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

#include "mod_pagespeed/mod_pagespeed.h"
#include "third_party/apache/httpd/src/include/httpd.h"
#include "third_party/apache/httpd/src/include/http_core.h"

namespace {

// All these constants are defaults for the conviniece of developing. They are
// sure not working on difference platforms or different configuration of
// systems. Use http.conf to configure those setting.
const char* kGeneratedFilePrefix = "/usr/local/apache2/htdocs/cache/cache_pre_";
const char* kUrlPrefix = "http://localhost:9999/cache/cache_pre_";
const char* kFileCachePath = "/tmp/html_rewrite_cache";
const char* kFetcherProxy = "localhost:9999";
const int64 kFetcherTimeOut = 30000;  // 30 seconds.
const int64 kResourceFetcherTimeOut = 300000;  // 5 minutes.
}  // namespace


namespace html_rewriter {

std::string GetCachePrefix(server_rec* server) {
  const char* generated_file_prefix =
      mod_pagespeed_get_config_str(server, kPagespeedGeneratedFilePrefix);
  return generated_file_prefix ? generated_file_prefix : kGeneratedFilePrefix;
}

const char* GetUrlPrefix(server_rec* server) {
  const char* url_prefix =
      mod_pagespeed_get_config_str(server, kPagespeedRewriteUrlPrefix);
  return url_prefix ? url_prefix : kUrlPrefix;
}

const char* GetFileCachePath(server_rec* server) {
  const char* file_cache_path =
      mod_pagespeed_get_config_str(server, kPagespeedFileCachePath);
  return file_cache_path ? file_cache_path : kFileCachePath;
}

const char* GetFetcherProxy(server_rec* server) {
  const char* fetch_proxy =
      mod_pagespeed_get_config_str(server, kPagespeedFetchProxy);
  return fetch_proxy ? fetch_proxy : kFetcherProxy;
}

int64 GetFetcherTimeOut(server_rec* server) {
  int64 time_out =
      mod_pagespeed_get_config_int(server, kPagespeedFetcherTimeoutMs);
  return time_out == -1 ? kFetcherTimeOut : time_out;
}

int64 GetResourceFetcherTimeOutMs(server_rec* server) {
  int64 time_out =
      mod_pagespeed_get_config_int(server, kPagespeedResourceTimeoutMs);
  return time_out == -1 ? kResourceFetcherTimeOut : time_out;
}

}  // namespace html_rewriter
