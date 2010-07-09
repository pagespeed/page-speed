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

#ifndef HTML_REWRITER_HTML_REWRITER_IMP_H_
#define HTML_REWRITER_HTML_REWRITER_IMP_H_

#include <string>

#include "html_rewriter/apr_file_system.h"
#include "html_rewriter/apr_timer.h"
#include "html_rewriter/html_parser_message_handler.h"
#include "html_rewriter/md5_hasher.h"
#include "html_rewriter/serf_url_async_fetcher.h"
#include "mod_pagespeed/pagespeed_server_context.h"
#include "net/instaweb/htmlparse/public/html_parse.h"
#include "net/instaweb/htmlparse/public/html_writer_filter.h"
#include "net/instaweb/rewriter/public/hash_resource_manager.h"
#include "net/instaweb/rewriter/public/rewrite_driver.h"
#include "net/instaweb/util/public/cache_url_async_fetcher.h"
#include "net/instaweb/util/public/cache_url_fetcher.h"
#include "net/instaweb/util/public/file_cache.h"
#include "net/instaweb/util/public/filename_encoder.h"
#include "net/instaweb/util/public/http_cache.h"
#include "net/instaweb/util/public/string_writer.h"
#include "net/instaweb/util/public/writer.h"


struct request_rec;

namespace html_rewriter {

// TODO(lsong): Make HtmlRewriterImp a re-usable object because creating an
// object for every request involves creating all the internal objects.
class HtmlRewriterImp {
 public:
  HtmlRewriterImp(request_rec* request,
                  const std::string& url, std::string* output);
  ~HtmlRewriterImp();
  // Rewrite input using internal StringWriter.
  void Rewrite(const char* input, int size);
  void Rewrite(const std::string& input) {
    Rewrite(input.data(), input.size());
  }
  // Flush the re-written content to output.
  void Flush();
  // Flush and finish the re-write.
  void Finish();

  const std::string& get_url() const {
    return url_;
  };
  void set_url(const std::string& url) {
    url_ = url;
  };
  // Call this function to wait all the asynchronous fetchers to finish.
  // In mod_pagespeed, this fucntion is call in the log_transaction hook.
  static void WaitForInProgressDownloads(request_rec* request);

 private:
  PageSpeedServerContext* context_;
  std::string url_;
  net_instaweb::RewriteDriver* rewrite_driver_;
  net_instaweb::StringWriter string_writer_;
};

}  // namespace html_rewriter

#endif  // HTML_REWRITER_HTML_REWRITER_IMP_H_
