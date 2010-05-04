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

#include "html_rewriter/html_rewriter.h"

#include "html_rewriter/html_rewriter_imp.h"

namespace html_rewriter {

HtmlRewriter::HtmlRewriter(request_rec* request,
                           const std::string& url, std::string* output)
    :html_rewriter_imp_(new HtmlRewriterImp(request, url, output)) {
}

HtmlRewriter::~HtmlRewriter() {
  delete html_rewriter_imp_;
}

void HtmlRewriter::Finish() {
  html_rewriter_imp_->Finish();
}

void HtmlRewriter::Flush() {
  html_rewriter_imp_->Flush();
}

void HtmlRewriter::Rewrite(const char* input, int size) {
  html_rewriter_imp_->Rewrite(input, size);
}

void HtmlRewriter::WaitForInProgressDownloads(request_rec* request) {
  HtmlRewriterImp::WaitForInProgressDownloads(request);
}

}  // namespace html_rewriter
