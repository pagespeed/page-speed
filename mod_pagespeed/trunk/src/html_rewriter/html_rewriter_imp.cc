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


HtmlRewriterImp::HtmlRewriterImp(request_rec* request,
                                 const std::string& url, std::string* output)
    : context_(GetPageSpeedProcessContext(request->server)),
      message_handler_(context_->message_handler()),
      file_system_(context_->file_system()),
      file_cache_(context_->file_cache()),
      http_cache_(context_->http_cache()),
      serf_url_async_fetcher_(context_->fetcher()),
      cache_url_fetcher_(context_->cache_url_fetcher()),
      cache_url_async_fetcher_(context_->cache_url_async_fetcher()),
      url_(url),
      html_parse_(message_handler_),
      rewrite_driver_(&html_parse_, file_system_,
                      cache_url_async_fetcher_),
      hash_resource_manager_(GetCachePrefix(request),
                             GetUrlPrefix(),
                             0,
                             true,
                             file_system_,
                             &filename_encoder_,
                             cache_url_fetcher_,
                             &md5_hasher_),
      string_writer_(output) {
  rewrite_driver_.SetResourceManager(&hash_resource_manager_);
  std::string base_url;
  if (request->parsed_uri.scheme != NULL) {
    base_url.append(request->parsed_uri.scheme);
  } else {
    base_url.append("http");
  }
  base_url.append("://");
  if (request->parsed_uri.hostinfo != NULL) {
    base_url.append(request->parsed_uri.hostinfo);
  } else {
    base_url.append(request->hostname);
    if (request->parsed_uri.port_str != NULL) {
      base_url.append(":");
      base_url.append(request->parsed_uri.port_str);
    }
  }
  if (request->parsed_uri.path != NULL) {
    base_url.append(request->parsed_uri.path);
  } else {
    base_url.append(request->uri);
  }
  rewrite_driver_.SetBaseUrl(base_url);
  rewrite_driver_.CombineCssFiles();
  rewrite_driver_.RewriteImages();
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
