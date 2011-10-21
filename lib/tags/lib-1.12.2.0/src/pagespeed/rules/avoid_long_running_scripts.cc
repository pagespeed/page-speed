// Copyright 2011 Google Inc. All Rights Reserved.
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

#include "pagespeed/rules/avoid_long_running_scripts.h"

#include "base/basictypes.h"
#include "base/logging.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/instrumentation_data.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/core/rule_input.h"
#include "pagespeed/l10n/l10n.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/proto/timeline.pb.h"

namespace pagespeed {

namespace {

// How long a script has to run for to be considered "long-running".
const double kLongScriptDuration = 100.0;  // milliseconds

class LongRunningScriptsVisitor : public InstrumentationDataVisitor {
 public:
  explicit LongRunningScriptsVisitor(ResultProvider* provider);
  virtual bool Visit(const InstrumentationDataStack& stack);

 private:
  ResultProvider* const provider_;

  DISALLOW_COPY_AND_ASSIGN(LongRunningScriptsVisitor);
};

LongRunningScriptsVisitor::LongRunningScriptsVisitor(ResultProvider* provider)
    : provider_(provider) {
  CHECK(NULL != provider_);
}

bool LongRunningScriptsVisitor::Visit(const InstrumentationDataStack& stack) {
  const InstrumentationData& event = *stack.back();
  if (event.type() != InstrumentationData::EVALUATE_SCRIPT &&
      event.type() != InstrumentationData::FUNCTION_CALL) {
    return true;  // visit children
  }

  if (!event.has_start_time() || !event.has_end_time()) {
    LOG(WARNING) << "EvaluateScript/FunctionCall event with no start/end time";
    return false;
  }

  const double duration = event.end_time() - event.start_time();
  if (duration < kLongScriptDuration) {
    return false;
  }

  if (!event.has_data()) {
    LOG(WARNING) << "EvaluateScript/FunctionCall event with no data dict";
    return false;
  }

  const InstrumentationData::DataDictionary& data = event.data();
  std::string url;
  int line_number;
  if (event.type() == InstrumentationData::EVALUATE_SCRIPT) {
    if (data.has_url()) {
      url = data.url();
    } else {
      LOG(WARNING) << "EvaluateScript event with no url";
      return false;
    }
    if (data.has_line_number()) {
      line_number = data.line_number();
    } else {
      LOG(WARNING) << "EvaluateScript event with no line number";
      return false;
    }
  } else if (event.type() == InstrumentationData::FUNCTION_CALL) {
    if (data.has_script_name()) {
      url = data.script_name();
    } else {
      LOG(WARNING) << "FunctionCall event with no script_name";
      return false;
    }
    if (data.has_script_line()) {
      line_number = data.script_line();
    } else {
      LOG(WARNING) << "FunctionCall event with no script_line";
      return false;
    }
  } else {
    DCHECK(false) << "unreachable";
    return false;
  }

  Result* result = provider_->NewResult();
  result->add_resource_urls(url);
  ResultDetails* details = result->mutable_details();
  AvoidLongRunningScriptsDetails* lrs_details =
      details->MutableExtension(
          AvoidLongRunningScriptsDetails::message_set_extension);
  lrs_details->set_duration_millis(duration);
  lrs_details->set_line_number(line_number);

  return false;  // don't visit children
}

}  // namespace

namespace rules {

AvoidLongRunningScripts::AvoidLongRunningScripts()
    : pagespeed::Rule(pagespeed::InputCapabilities(
        pagespeed::InputCapabilities::TIMELINE_DATA)) {}

const char* AvoidLongRunningScripts::name() const {
  return "AvoidLongRunningScripts";
}

UserFacingString AvoidLongRunningScripts::header() const {
  // TRANSLATOR: The name of a Page Speed rule that tells users to try
  // and avoid writing pages with Javascript scripts that run for a
  // long time, which reduces browser responsiveness.  This is
  // displayed in a list of rule names that Page Speed generates,
  // telling webmasters which rules they broke in their website.
  return _("Avoid long-running scripts");
}

bool AvoidLongRunningScripts::AppendResults(const RuleInput& rule_input,
                                            ResultProvider* provider) {
  const PagespeedInput& input = rule_input.pagespeed_input();
  const InstrumentationDataVector* timeline = input.instrumentation_data();
  CHECK(NULL != timeline);

  LongRunningScriptsVisitor visitor(provider);
  InstrumentationDataVisitor::Traverse(&visitor, *timeline);

  return true;
}

void AvoidLongRunningScripts::FormatResults(const ResultVector& results,
                                            RuleFormatter* formatter) {
  if (results.empty()) {
    return;
  }

  UrlBlockFormatter* body = formatter->AddUrlBlock(
      // TRANSLATOR: Heading that gives a high-level overview of the
      // reason suggestions are being made.
      _("The following URLs run JavaScript that blocks the UI for a long "
        "time. To improve browser responsiveness, optimize the JavaScript "
        "or split it up using callbacks."));

  for (ResultVector::const_iterator iter = results.begin(),
           end = results.end();
       iter != end;
       ++iter) {
    const Result& result = **iter;
    if (result.resource_urls_size() != 1) {
      LOG(DFATAL) << "Unexpected number of resource URLs.  Expected 1, Got "
                  << result.resource_urls_size() << ".";
      continue;
    }

    const ResultDetails& details = result.details();
    if (!details.HasExtension(
            AvoidLongRunningScriptsDetails::message_set_extension)) {
      LOG(DFATAL) << "AvoidLongRunningScriptsDetails missing.";
      continue;
    }

    const AvoidLongRunningScriptsDetails& lrs_details =
        details.GetExtension(
            AvoidLongRunningScriptsDetails::message_set_extension);

    body->AddUrlResult(
        // TRANSLATOR: Shown as part of a list of URLs that initiate JavaScript
        // scripts that run for a long time.  Shows a URL at $1, the line
        // number (of the file at that URL that triggers the script URL) at $2,
        // and the length of time the scripts runs for at $3.
        _("$1 line $2 ($3)"),
        UrlArgument(result.resource_urls(0)),
        IntArgument(lrs_details.line_number()),
        DurationArgument(static_cast<int64>(lrs_details.duration_millis())));
  }
}

bool AvoidLongRunningScripts::IsExperimental() const {
  // TODO(mdsteele): Before graduating from experimental:
  // 1. implement ComputeScore
  // 2. implement ComputeResultImpact
  return true;
}

}  // namespace rules

}  // namespace pagespeed
