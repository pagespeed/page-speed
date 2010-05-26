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

#include "html_rewriter/apr_file_system.h"
#include "html_rewriter/apr_timer.h"
#include "html_rewriter/html_parser_message_handler.h"
#include "html_rewriter/html_rewriter_config.h"
#include "html_rewriter/serf_url_async_fetcher.h"
#include "net/instaweb/util/public/cache_url_async_fetcher.h"
#include "net/instaweb/util/public/cache_url_fetcher.h"
#include "net/instaweb/util/public/file_cache.h"
#include "net/instaweb/util/public/http_cache.h"
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
}

void PageSpeedProcessContext::set_fetcher(SerfUrlAsyncFetcher* fetcher) {
  fetcher_.reset(fetcher);
}

void PageSpeedProcessContext::set_file_system(
  net_instaweb::FileSystem* file_system) {
  file_system_.reset(file_system);
}

void PageSpeedProcessContext::set_message_handler(
  net_instaweb::MessageHandler* handler) {
  message_handler_.reset(handler);
}

void PageSpeedProcessContext::set_file_cache(
  net_instaweb::FileCache* file_cache) {
  file_cache_.reset(file_cache);
}

void PageSpeedProcessContext::set_timer(
  net_instaweb::Timer* timer) {
  timer_.reset(timer);
}

void PageSpeedProcessContext::set_http_cache(
  net_instaweb::HTTPCache* http_cache) {
  http_cache_.reset(http_cache);
}

void PageSpeedProcessContext::set_cache_url_fetcher(
  net_instaweb::CacheUrlFetcher* cache_url_fetcher) {
  cache_url_fetcher_.reset(cache_url_fetcher);
}

void PageSpeedProcessContext::set_cache_url_async_fetcher(
  net_instaweb::CacheUrlAsyncFetcher* cache_url_async_fetcher) {
  cache_url_async_fetcher_.reset(cache_url_async_fetcher);
}


const PageSpeedProcessContext* GetPageSpeedProcessContext(server_rec* server) {
  const PageSpeedProcessContext* context =
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
  context->set_fetcher(new SerfUrlAsyncFetcher(GetFetcherProxy().c_str()));
  context->set_file_system(new AprFileSystem(NULL));
  context->set_timer(new AprTimer());
  context->set_message_handler(new HtmlParserMessageHandler());
  context->set_file_cache(
      new net_instaweb::FileCache(GetFileCachePath(),
                                  context->file_system(),
                                  context->message_handler()));
  context->set_http_cache(new net_instaweb::HTTPCache(context->file_cache(),
                                                      context->timer()));
  context->set_cache_url_fetcher(
      new net_instaweb::CacheUrlFetcher(context->http_cache(),
                                        context->fetcher()));
  context->set_cache_url_async_fetcher(
      new net_instaweb::CacheUrlAsyncFetcher(context->http_cache(),
                                             context->fetcher()));
}

}  // namespace html_rewriter
