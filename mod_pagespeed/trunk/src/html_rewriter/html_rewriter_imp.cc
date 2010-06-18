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
#include "html_rewriter/apache_rewrite_driver_factory.h"
#include "html_rewriter/html_rewriter_config.h"
#include "mod_spdy/apache/log_message_handler.h"
#include "third_party/apache/httpd/src/include/httpd.h"


namespace html_rewriter {


HtmlRewriterImp::HtmlRewriterImp(request_rec* request,
                                 const std::string& url, std::string* output)
    : context_(GetPageSpeedProcessContext(request->server)),
      url_(url),
      rewrite_driver_(context_->rewrite_driver_factory()->GetRewriteDriver()),
      string_writer_(output) {
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
  rewrite_driver_->SetBaseUrl(base_url);
  // TODO(lsong): Bypass the string buffer, writer data directly to the next
  // apache bucket.
  rewrite_driver_->SetWriter(&string_writer_);
  rewrite_driver_->html_parse()->StartParse(url_.c_str());
}

HtmlRewriterImp::~HtmlRewriterImp() {
  context_->rewrite_driver_factory()->ReleaseRewriteDriver(rewrite_driver_);
}

void HtmlRewriterImp::Finish() {
  rewrite_driver_->html_parse()->FinishParse();
}

void HtmlRewriterImp::Flush() {
  rewrite_driver_->html_parse()->Flush();
}

void HtmlRewriterImp::Rewrite(const char* input, int size) {
  rewrite_driver_->html_parse()->ParseText(input, size);
}

}  // namespace html_rewriter
