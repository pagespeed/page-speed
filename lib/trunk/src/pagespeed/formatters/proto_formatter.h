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

// Author: aoates@google.com (Andrew Oates)

#ifndef PAGESPEED_FORMATTERS_LOCALIZED_FORMATTER_H_
#define PAGESPEED_FORMATTERS_LOCALIZED_FORMATTER_H_

#include "pagespeed/core/formatter.h"
#include "pagespeed/core/serializer.h"

namespace pagespeed {

class FormattedResults;
namespace l10n { class Localizer; }

namespace formatters {

// TODO(aoates): should the localization functionality  be moved into the base
// Formatter class, so that all formatters are supplied transparently with
// localized strings?
/**
 * Formatter that fills in a localized FormattedResults proto.
 */
class ProtoFormatter : public RuleFormatter {
 public:
  ProtoFormatter(const pagespeed::l10n::Localizer* localizer,
                     FormattedResults* results);

  // RuleFormatter interface.
  virtual Formatter* AddHeader(const Rule& rule, int score);

 protected:
  // Formatter interface.
  virtual Formatter* NewChild(const FormatterParameters& params);
  virtual void DoneAddingChildren();

 private:
  const pagespeed::l10n::Localizer* localizer_;
  FormattedResults* results_;
};

}  // namespace formatters

}  // namespace pagespeed

#endif  // PAGESPEED_FORMATTERS_LOCALIZED_FORMATTER_H_
