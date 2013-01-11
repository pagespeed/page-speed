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

#include "pagespeed/rules/eliminate_unnecessary_reflows.h"

#include <algorithm>

#include "base/logging.h"
#include "base/stl_util.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/instrumentation_data.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/core/rule_input.h"
#include "pagespeed/core/string_util.h"
#include "pagespeed/l10n/l10n.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/proto/timeline.pb.h"

namespace {

using pagespeed::EliminateUnnecessaryReflowsDetails;
using pagespeed::EliminateUnnecessaryReflowsDetails_StackTrace;
using pagespeed::InstrumentationData;
using pagespeed::InstrumentationDataStack;
using pagespeed::InstrumentationDataStackVector;
using pagespeed::InstrumentationDataVisitor;

struct StackTraceLessThan;
typedef std::map<std::string,  InstrumentationDataStackVector>
    URLToInstrumentationStackVectorMap;
typedef std::set<EliminateUnnecessaryReflowsDetails_StackTrace*,
                 StackTraceLessThan> StackTraceSet;

// When displaying function names in stack traces, we reserve at least
// 10 characters and at most 75 characters (truncating if function
// names are longer than 75 characters).
static const int kMinFunctionNameWidth = 10;
static const int kMaxFunctionNameWidth = 75;

// Comparator for stack traces. The specific sort order here is not
// important; this is used only to identify identical stack traces.
struct StackTraceLessThan {
  bool operator() (
      const EliminateUnnecessaryReflowsDetails_StackTrace* lhs,
      const EliminateUnnecessaryReflowsDetails_StackTrace* rhs) const {
    if (lhs->frame_size() != rhs->frame_size()) {
      return lhs->frame_size() < rhs->frame_size();
    }
    for (int i = 0; i < lhs->frame_size(); ++i) {
      const pagespeed::StackFrame& left_frame = lhs->frame(i);
      const pagespeed::StackFrame& right_frame = rhs->frame(i);
      if (left_frame.column_number() != right_frame.column_number()) {
        return left_frame.column_number() < right_frame.column_number();
      }
      if (left_frame.line_number() != right_frame.line_number()) {
        return left_frame.line_number() < right_frame.line_number();
      }
      if (left_frame.function_name() != right_frame.function_name()) {
        return left_frame.function_name() < right_frame.function_name();
      }
      if (left_frame.url() != right_frame.url()) {
        return left_frame.url() < right_frame.url();
      }
    }

    // The two frames are exactly equivalent. Return false since lhs
    // is not less than rhs.
    return false;
  }
};

const EliminateUnnecessaryReflowsDetails* GetDetails(
    const pagespeed::Result& result) {
  const pagespeed::ResultDetails& details = result.details();
  if (!details.HasExtension(
          EliminateUnnecessaryReflowsDetails::message_set_extension)) {
    LOG(DFATAL) << "EliminateUnnecessaryReflowsDetails missing.";
    return NULL;
  }

  return &details.GetExtension(
      EliminateUnnecessaryReflowsDetails::message_set_extension);
}

// Sort stack traces by their runtimes, in order to present the traces
// that executed most first in the UI.
bool SortStackTracesByDuration(
    const EliminateUnnecessaryReflowsDetails_StackTrace* lhs,
    const EliminateUnnecessaryReflowsDetails_StackTrace* rhs) {
  return lhs->duration_millis() > rhs->duration_millis();
}

bool SortRuleResultsByDuration(const pagespeed::Result* lhs,
                               const pagespeed::Result* rhs) {
  const pagespeed::EliminateUnnecessaryReflowsDetails* lhs_details =
      GetDetails(*lhs);
  double lhs_duration = 0;
  for (int i = 0; i < lhs_details->stack_trace_size(); ++i) {
    lhs_duration += lhs_details->stack_trace(i).duration_millis();
  }

  const pagespeed::EliminateUnnecessaryReflowsDetails* rhs_details =
      GetDetails(*rhs);
  double rhs_duration = 0;
  for (int i = 0; i < rhs_details->stack_trace_size(); ++i) {
    rhs_duration += rhs_details->stack_trace(i).duration_millis();
  }

  return lhs_duration > rhs_duration;
}

// Get the URL of the root resource that triggered the reflow. This
// may be different from the URL at the bottom of the JS call stack
// since the JS call stacks are truncated to the most recent 5 frames.
bool GetRootJavaScriptUrl(
    const InstrumentationDataStack& stack, std::string* out) {
  for (InstrumentationDataStack::const_iterator
           it2 = stack.begin(), end2 = stack.end(); it2 != end2; ++it2) {
    const InstrumentationData& data = **it2;
    if (data.type() == InstrumentationData::FUNCTION_CALL) {
      if (data.data().has_script_name()) {
        *out = data.data().script_name();
        return true;
      }
    } else if (data.type() == InstrumentationData::EVALUATE_SCRIPT) {
      if (data.data().has_url()) {
        *out = data.data().url();
        return true;
      }
    }
  }
  return false;
}

// Format a strack trace in a way that is human readable.
std::string GetPresentableStackTrace(
    const EliminateUnnecessaryReflowsDetails_StackTrace& stack) {
  std::vector<std::string> trace;
  int max_function_name_width = kMinFunctionNameWidth;
  for (int i = 0; i < stack.frame_size(); ++i) {
    const int function_name_width = stack.frame(i).function_name().size();
    if (function_name_width > max_function_name_width) {
      max_function_name_width = function_name_width;
    }
  }
  if (max_function_name_width > kMaxFunctionNameWidth) {
    max_function_name_width = kMaxFunctionNameWidth;
  }
  for (int i = 0; i < stack.frame_size(); ++i) {
    const pagespeed::StackFrame& frame = stack.frame(i);
    trace.push_back(pagespeed::string_util::StringPrintf(
        "%*s @ %.75s:%d:%d",
        max_function_name_width,
        frame.function_name().c_str(),
        frame.url().c_str(),
        frame.line_number(),
        frame.column_number()));
  }
  return pagespeed::string_util::JoinString(trace, '\n');
}

// Find all unique stack traces within the
// InstrumentationDataStackVector, and compute the number of times
// that each trace was encountered.
static int ComputeUniqueStackTraces(
    const InstrumentationDataStackVector& stacks, StackTraceSet* trace_set) {
  int num_reflows = 0;
  for (InstrumentationDataStackVector::const_iterator
           it = stacks.begin(), end = stacks.end(); it != end; ++it) {
    const InstrumentationData& layout = *it->back();

    // Sometimes, layout nodes cause their parents to perform layouts
    // as well. We want to find the rootmost layout node that is a
    // parent of this layout node.
    const InstrumentationData* root_layout = &layout;
    for (InstrumentationDataStack::const_reverse_iterator
             rit = it->rbegin(), rend = it->rend(); rit != rend; ++rit) {
      const InstrumentationData* candidate = *rit;
      if (candidate->type() != InstrumentationData::LAYOUT) {
        break;
      }
      root_layout = candidate;
    }
    const double duration = root_layout->end_time() - root_layout->start_time();
    if (duration < 1.0) {
      // Skip over traces with very short durations.
      continue;
    }
    scoped_ptr<EliminateUnnecessaryReflowsDetails_StackTrace> trace(
        new EliminateUnnecessaryReflowsDetails_StackTrace());
    for (int idx = 0; idx < layout.stack_trace_size(); ++idx) {
      trace->add_frame()->MergeFrom(layout.stack_trace(idx));
    }
    // If a matching stack trace is already in the set, increment its
    // count. Otherwise add this trace and set the count to 1.
    StackTraceSet::const_iterator trace_iter = trace_set->find(trace.get());
    if (trace_iter != trace_set->end()) {
      EliminateUnnecessaryReflowsDetails_StackTrace* trace2 = *trace_iter;
      trace2->set_count(trace2->count() + 1);
      trace2->set_duration_millis(trace2->duration_millis() + duration);
    } else {
      trace->set_count(1);
      trace->set_duration_millis(duration);
      trace_set->insert(trace.release());
    }
    ++num_reflows;
  }
  return num_reflows;
}

// InstrumentationDataVisitor that finds call stacks that triggered
// unnecessary reflows.
class UnnecessaryReflowDiscoverer : public InstrumentationDataVisitor {
 public:
  UnnecessaryReflowDiscoverer(URLToInstrumentationStackVectorMap* out)
      : root_to_layout_stack_map_(out) {}

  virtual bool Visit(const InstrumentationDataStack& stack);

 private:
  URLToInstrumentationStackVectorMap* root_to_layout_stack_map_;

  DISALLOW_COPY_AND_ASSIGN(UnnecessaryReflowDiscoverer);
};

bool UnnecessaryReflowDiscoverer::Visit(const InstrumentationDataStack& stack) {
  const InstrumentationData& data = *stack.back();
  if (data.type() != InstrumentationData::LAYOUT) {
    return true;
  }

  if (data.stack_trace_size() == 0) {
    // This is a layout node without a stack trace, which means it
    // wasn't triggered synchronously by JavaScript code. We should
    // skip it, but it might have a child with a trace, so return true
    // to visit children.
    return true;
  }

  std::string url;
  if (GetRootJavaScriptUrl(stack, &url)) {
    (*root_to_layout_stack_map_)[url].push_back(stack);
  }
  return true;
}

}  // namespace

namespace pagespeed {

namespace rules {

EliminateUnnecessaryReflows::EliminateUnnecessaryReflows()
    : pagespeed::Rule(pagespeed::InputCapabilities(
        pagespeed::InputCapabilities::TIMELINE_DATA)) {
}

const char* EliminateUnnecessaryReflows::name() const {
  return "EliminateUnnecessaryReflows";
}

UserFacingString EliminateUnnecessaryReflows::header() const {
  // TRANSLATOR: Title of this rule, which suggests
  // removing/eliminating reflows (or "layouts", which is used
  // interchangeably with the word "reflows" in the web performance
  // community).
  return _("Eliminate unnecessary reflows");
}

bool EliminateUnnecessaryReflows::AppendResults(const RuleInput& rule_input,
                                                ResultProvider* provider) {
  const PagespeedInput& input = rule_input.pagespeed_input();

  // 1. Find all unnecessary reflows, grouped by the URL of the
  // resource that triggered them.
  URLToInstrumentationStackVectorMap m;
  UnnecessaryReflowDiscoverer visitor(&m);
  InstrumentationDataVisitor::Traverse(&visitor, *input.instrumentation_data());

  for (URLToInstrumentationStackVectorMap::const_iterator
           stack_vector_iter = m.begin(), end = m.end();
       stack_vector_iter != end; ++stack_vector_iter) {
    const std::string& url = stack_vector_iter->first;
    const Resource* resource = input.GetResourceWithUrlOrNull(url);
    if (resource == NULL) {
      // Only include results for resources that were included in the
      // input.
      LOG(INFO) << "Unable to find resource with url " << url;
      continue;
    }

    // 2. Group the unnecessary reflows by their stack traces.
    StackTraceSet trace_set;
    STLElementDeleter<StackTraceSet> deleter(&trace_set);
    int num_reflows = ComputeUniqueStackTraces(
        stack_vector_iter->second, &trace_set);
    if (num_reflows == 0) {
      continue;
    }

    // 3. Generate results, one per root resource URL.
    Result* result = provider->NewResult();
    Savings* savings = result->mutable_savings();
    // TODO(bmcquade): page_reflows_saved is not a great metric since
    // there is no indication of the cost of the reflow. Revisit this
    // before graduating this rule from experimental status and
    // compute a more accurate impact for each reflow suggestion.
    savings->set_page_reflows_saved(num_reflows);
    result->add_resource_urls(resource->GetRequestUrl());
    ResultDetails* details = result->mutable_details();
    EliminateUnnecessaryReflowsDetails* eur_details =
        details->MutableExtension(
            EliminateUnnecessaryReflowsDetails::message_set_extension);

    // Store the unique stack traces for this resource.
    for (StackTraceSet::const_iterator trace_iter = trace_set.begin(),
             end3 = trace_set.end(); trace_iter != end3; ++trace_iter) {
      // Other interesting statistics to consider including in the future:
      // * before/after DOMContentLoaded?
      // * seconds after load start?
      eur_details->add_stack_trace()->MergeFrom(**trace_iter);
    }
  }
  return true;
}

void EliminateUnnecessaryReflows::FormatResults(const ResultVector& results,
                                                RuleFormatter* formatter) {
  typedef std::vector<const EliminateUnnecessaryReflowsDetails_StackTrace*>
      StackTraceVector;

  if (results.empty()) {
    return;
  }

  UrlBlockFormatter* body = formatter->AddUrlBlock(
      // TRANSLATOR: Heading that gives a high-level overview of the
      // reason suggestions are being made, as well as a brief summary
      // of how to fix the issue. Beneath this heading, the JavaScript
      // for specific instances of unnecessary reflows (or "layouts",
      // which is used interchangeably with the word "reflows" in the
      // web performance community) that happened on the page being
      // analyzed will be shown.
      _("JavaScript that executed in the following resources caused "
        "unnecessary reflows. To reduce page render time, modify the "
        "JavaScript so it does not cause a reflow:"));

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

    const EliminateUnnecessaryReflowsDetails* eur_details = GetDetails(result);
    if (eur_details == NULL) {
      continue;
    }

    UrlFormatter* url_formatter = body->AddUrlResult(
        // TRANSLATOR: Shown as part of a list of unnecessary reflows
        // (or "layouts", which is used interchangeably with the word
        // "reflows" in the web performance community). Shows a URL
        // at $1, and the number of unnecessary reflows for that URL
        // at $2.
        _("$1 ($2 reflows)"), UrlArgument(result.resource_urls(0)),
        IntArgument(result.savings().page_reflows_saved()));
    StackTraceVector traces;
    for (int i = 0; i < eur_details->stack_trace_size(); ++i) {
      traces.push_back(&eur_details->stack_trace(i));
    }
    std::sort(traces.begin(), traces.end(), SortStackTracesByDuration);
    for (StackTraceVector::const_iterator stack_iter = traces.begin(),
             end = traces.end(); stack_iter != end; ++stack_iter) {
      const EliminateUnnecessaryReflowsDetails::StackTrace& stack =
          **stack_iter;
      if (stack.count() == 1) {
        url_formatter->AddDetail(
            // TRANSLATOR: Appears as part of the list of URLs that
            // triggered unnecessary reflows (or "layouts", which is
            // used interchangeably with the word "reflows" in the web
            // performance community), as a detail string that shows
            // the context that caused an unnecessary reflow.  $1
            // contains JavaScript code that gives the context of the
            // reflow.
            _("The following JavaScript call stack caused a reflow that "
              "took $1 milliseconds: $2"),
            IntArgument(static_cast<int64>(stack.duration_millis())),
            VerbatimStringArgument(GetPresentableStackTrace(stack)));
      } else {
        url_formatter->AddDetail(
            // TRANSLATOR: Appears as part of the list of URLs that
            // triggered unnecessary reflows (or "layouts", which is
            // used interchangeably with the word "reflows" in the web
            // performance community), as a detail string that shows
            // the context that caused an unnecessary reflow.  $1
            // contains the number of times the code in this context
            // executed, and $2 contains JavaScript code that gives
            // the context of the reflow.
            _("The following JavaScript call stack (executed $1 times) "
              "caused reflows that took $2 milliseconds: $3"),
            IntArgument(stack.count()),
            IntArgument(static_cast<int64>(stack.duration_millis())),
            VerbatimStringArgument(GetPresentableStackTrace(stack)));
      }
    }
  }
}

void EliminateUnnecessaryReflows::SortResultsInPresentationOrder(
    ResultVector* rule_results) const {
  // Sort the results in a consistent order so they're always
  // presented to the user in the same order.
  std::stable_sort(rule_results->begin(),
                   rule_results->end(),
                   SortRuleResultsByDuration);
}

bool EliminateUnnecessaryReflows::IsExperimental() const {
  // TODO(bmcquade): Before graduating from experimental:
  // 1. implement ComputeScore
  // 2. implement ComputeResultImpact
  return true;
}

}  // namespace rules

}  // namespace pagespeed
