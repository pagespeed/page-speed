// Copyright 2010 Google Inc.
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

#include "mod_pagespeed/pagespeed_process_context.h"

#include "html_rewriter/html_rewriter_config.h"
#include "html_rewriter/serf_url_async_fetcher.h"
#include "third_party/apache_httpd/include/httpd.h"
#include "third_party/apache_httpd/include/http_config.h"
#include "third_party/apache_httpd/include/http_log.h"
#include "third_party/apache_httpd/include/http_protocol.h"

extern module AP_MODULE_DECLARE_DATA pagespeed_module;

namespace html_rewriter {

PageSpeedProcessContext::PageSpeedProcessContext()
    : fetcher_(NULL) {
}

PageSpeedProcessContext::~PageSpeedProcessContext() {
  if (fetcher_ != NULL) {
    delete fetcher_;
  }
}
PageSpeedProcessContext* GetPageSpeedProcessContext(server_rec* server) {
  PageSpeedProcessContext* context =
      static_cast<PageSpeedProcessContext*>(
          ap_get_module_config(server->module_config, &pagespeed_module));
  return context;
}

SerfUrlAsyncFetcher* GetSerfAsyncFetcher(server_rec* server) {
  PageSpeedProcessContext* context = GetPageSpeedProcessContext(server);
  if (context != NULL) {
    return context->fetcher();
  } else {
    return NULL;
  }
}

void CreateSerfAsyncFetcher(server_rec* server) {
  PageSpeedProcessContext* context =
      static_cast<PageSpeedProcessContext*>(
          ap_get_module_config(server->module_config, &pagespeed_module));
  if (context == NULL) {
    context = new PageSpeedProcessContext;
    ap_set_module_config(server->module_config, &pagespeed_module, context);
  }
  if (context->fetcher() == NULL) {
    SerfUrlAsyncFetcher* fetcher =
        new SerfUrlAsyncFetcher(GetFetcherProxy().c_str());
    context->set_fetcher(fetcher);
  }
}

}  // namespace html_rewriter
