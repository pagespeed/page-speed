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

#include "third_party/apache/httpd/src/include/httpd.h"
#include "third_party/apache/httpd/src/include/http_config.h"
#include "third_party/apache/httpd/src/include/http_log.h"
#include "third_party/apache/httpd/src/include/http_protocol.h"

using net_instaweb::ApacheRewriteDriverFactory;

extern module AP_MODULE_DECLARE_DATA pagespeed_module;

namespace html_rewriter {

PageSpeedProcessContext::PageSpeedProcessContext() {
}

PageSpeedProcessContext::~PageSpeedProcessContext() {
}

PageSpeedProcessContext* GetPageSpeedProcessContext(server_rec* server) {
  PageSpeedProcessContext* context =
      static_cast<PageSpeedProcessContext*>(
          ap_get_module_config(server->module_config, &pagespeed_module));
  return context;
}

void CreatePageSpeedProcessContext(server_rec* server) {
  // TODO(lsong): thread-safty.
  PageSpeedProcessContext* context =
      static_cast<PageSpeedProcessContext*>(
          ap_get_module_config(server->module_config, &pagespeed_module));
  if (context == NULL) {
    context = new PageSpeedProcessContext;
    ap_set_module_config(server->module_config, &pagespeed_module, context);
  } else {
    ap_log_error(APLOG_MARK, APLOG_ERR, APR_SUCCESS, server,
                 "Process context is not NULL before creating.");
  }
  context->set_rewrite_driver_factory(new ApacheRewriteDriverFactory());
  context->rewrite_driver_factory()->set_combine_css(true);
  context->rewrite_driver_factory()->set_use_http_cache(true);
  // TODO(lsong): Make the rewriter_driver to use configurations from
  // httpd.conf.
  context->rewrite_driver_factory()->MakeRewriteDriver();
}

}  // namespace html_rewriter
