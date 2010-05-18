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


#include "html_rewriter/html_rewriter_imp.h"

#include <string>

#include "base/logging.h"
#include "html_rewriter/html_rewriter_config.h"
#include "mod_spdy/apache/log_message_handler.h"
#include "third_party/apache_httpd/include/httpd.h"


namespace html_rewriter {

SerfUrlAsyncFetcher* GetSerfAsyncFetcher(server_rec* server);

HtmlRewriterImp::HtmlRewriterImp(request_rec* request,
                                 const std::string& url, std::string* output)
    : url_(url),
      message_handler_(),
      html_parse_(&message_handler_),
      apr_file_system_(request->pool),
      file_cache_(GetFileCachePath(request), &apr_file_system_,
                  &message_handler_),
      http_cache_(&file_cache_, &apr_timer_),
      serf_url_async_fetcher_(GetSerfAsyncFetcher(request->server)),
      cache_url_fetcher_(&http_cache_, serf_url_async_fetcher_),
      cache_url_async_fetcher_(&http_cache_, serf_url_async_fetcher_),
      rewrite_driver_(&html_parse_, &cache_url_async_fetcher_),
      hash_resource_manager_(GetCachePrefix(request),
                             GetUrlPrefix(request),
                             0,
                             true,
                             &apr_file_system_,
                             &filename_encoder_,
                             &cache_url_fetcher_,
                             &md5_hasher_),
      string_writer_(output) {
  rewrite_driver_.SetResourceManager(&hash_resource_manager_);
  rewrite_driver_.CombineCssFiles();
  rewrite_driver_.RewriteImages();
  rewrite_driver_.RemoveQuotes();
  rewrite_driver_.SetWriter(&string_writer_);
  html_parse_.StartParse(url_.c_str());
}

void HtmlRewriterImp::Finish() {
  html_parse_.FinishParse();
}

void HtmlRewriterImp::Flush() {
  html_parse_.Flush();
}

void HtmlRewriterImp::Rewrite(const char* input, int size) {
  html_parse_.ParseText(input, size);
}

}  // namespace html_rewriter
