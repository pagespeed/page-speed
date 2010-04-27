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

#ifndef PAGESPEED_RULES_PUT_CSS_IN_THE_DOCUMENT_HEAD_H_
#define PAGESPEED_RULES_PUT_CSS_IN_THE_DOCUMENT_HEAD_H_

#include "base/basictypes.h"
#include "pagespeed/core/rule.h"

namespace pagespeed {

namespace rules {

/**
 * Lint rule that complains if style tags or stylesheet links appear in the
 * body.
 */
class PutCssInTheDocumentHead : public Rule {
 public:
  PutCssInTheDocumentHead();

  // Rule interface.
  virtual const char* name() const;
  virtual const char* header() const;
  virtual const char* documentation_url() const;
  virtual bool AppendResults(const PagespeedInput& input,
                             ResultProvider* provider);
  virtual void FormatResults(const ResultVector& results, Formatter* formatter);

 private:
  DISALLOW_COPY_AND_ASSIGN(PutCssInTheDocumentHead);
};

}  // namespace rules

}  // namespace pagespeed

#endif  // PAGESPEED_RULES_PUT_CSS_IN_THE_DOCUMENT_HEAD_H_
