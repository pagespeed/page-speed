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

#include "pagespeed/rules/avoid_excess_serialization.h"

#include <map>
#include <set>
#include <vector>

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

using pagespeed::InstrumentationData_DataDictionary;
using pagespeed::StackFrame;
using pagespeed::InstrumentationData;
using pagespeed::PagespeedInput;
using pagespeed::Resource;

namespace {

using ::google::protobuf::RepeatedPtrField;
using pagespeed::InstrumentationDataVector;
using pagespeed::InstrumentationDataVisitor;

typedef std::set<std::string> DependencySet;
typedef std::vector<std::string> DependencyTrace;
typedef std::map<std::string, DependencySet> DependencyMap;

// This constant specifies the minimum nesting level of requests which is
// allowed before the rule triggers.
static const unsigned int kMinNestingLevel = 3;

// Helper class to analyze the nesting level of requests.
class RequestAnalyzer : private InstrumentationDataVisitor {
public:
  RequestAnalyzer(const PagespeedInput* input) : input_(input) {}

  // Initializes this instances. This method must be called once before calling
  // GetDependencyTrace.
  void Init();

  // Returns a DependencyTrace describing the resources which were loaded
  // before the specified resource was loaded. The specified resource itself
  // is always the first entry in the trace.
  const DependencyTrace GetDependencyTrace(const Resource& resource);

private:
  virtual bool Visit(const std::vector<const InstrumentationData*>& stack);

  void OnResourceSendRequest(const InstrumentationData& record);

  DependencySet GetResources(const InstrumentationData& record);
  void FindLongestPath(const std::string& resource,
                       DependencySet* visited,
                       DependencyTrace* trace);

private:
  const PagespeedInput* input_;
  DependencyMap parent_resources_;

  // Stores the resolved dependency traces per resource URL.
  std::map<std::string, const DependencyTrace> dependency_traces_;
};

void RequestAnalyzer::Init() {
  const InstrumentationDataVector *instrumentation_data =
      input_->instrumentation_data();
  InstrumentationDataVisitor::Traverse(this, *instrumentation_data);
}

const DependencyTrace RequestAnalyzer::GetDependencyTrace(
    const Resource& resource) {
  DependencySet visited;
  DependencyTrace trace;
  FindLongestPath(resource.GetRequestUrl(), &visited, &trace);
  return trace;
}

bool RequestAnalyzer::Visit(
    const std::vector<const InstrumentationData*>& stack) {
  const InstrumentationData* record = *stack.rbegin();
  if (record->type() == InstrumentationData::RESOURCE_SEND_REQUEST) {
    OnResourceSendRequest(*record);
  }
  return true;
}

void RequestAnalyzer::FindLongestPath(const std::string& resource,
                                      DependencySet* visited,
                                      DependencyTrace* trace) {
  if (dependency_traces_.count(resource) == 0) {
    // We do not have the DependencyTrace for this resource in the cache yet.
    if (visited->count(resource) != 0) {
      // If we saw this resource before while trying to calculate the
      // DependencyTrace. This indicates a cycle in the graph.
      LOG(INFO) << "Request dependency graph is cyclic";
      return;
    }

    visited->insert(resource);

    DependencyTrace parent_trace;

    const DependencySet& rs = parent_resources_[resource];
    for (DependencySet::const_iterator iter = rs.begin();
        iter != rs.end(); ++iter) {

      DependencyTrace candidate_trace;
      FindLongestPath(*iter, visited, &candidate_trace);

      if (candidate_trace.size() > parent_trace.size()) {
        parent_trace = candidate_trace;
      }
    }

    visited->erase(resource);

    // this trace contains the resource itself.
    DependencyTrace this_trace;
    this_trace.push_back(resource);
    this_trace.insert(this_trace.end(),
                      parent_trace.begin(),
                      parent_trace.end());
    DCHECK(dependency_traces_.count(resource) == 0);
    dependency_traces_.insert(
        std::pair<std::string, const DependencyTrace>(resource, this_trace));
  }

  DCHECK(dependency_traces_.count(resource) == 1);
  (*trace) = dependency_traces_[resource];
}

void RequestAnalyzer::OnResourceSendRequest(const InstrumentationData& record) {
  const InstrumentationData_DataDictionary &data = record.data();

  std::string own_url = data.url();
  DependencySet all_resources = GetResources(record);
  parent_resources_[own_url].insert(all_resources.begin(), all_resources.end());
}

DependencySet RequestAnalyzer::GetResources(const InstrumentationData& record) {
  DependencySet result;
  const RepeatedPtrField<StackFrame>& stack_trace = record.stack_trace();
  for (RepeatedPtrField<StackFrame>::const_iterator iter =
      stack_trace.begin(); iter != stack_trace.end(); ++iter) {
    if (iter->has_url()) {
      result.insert(iter->url());
    }
  }

  return result;
}

} // namespace

namespace pagespeed {

namespace rules {

AvoidExcessSerialization::AvoidExcessSerialization()
    : pagespeed::Rule(pagespeed::InputCapabilities(
        pagespeed::InputCapabilities::TIMELINE_DATA)) {
}

const char* AvoidExcessSerialization::name() const {
  return "AvoidExcessSerialization";
}

UserFacingString AvoidExcessSerialization::header() const {
  // TRANSLATOR: The name of a Page Speed rule that tells users to try to avoid
  // serializing requests too excessively but rather try to parallelize them.
  // This is displayed in a list of rule names that Page Speed generates,
  // telling webmasters which rules they broke in their website.
  return _("Reduce request serialization");
}

bool AvoidExcessSerialization::AppendResults(const RuleInput& rule_input,
                                             ResultProvider* provider) {
  const PagespeedInput& input = rule_input.pagespeed_input();
  RequestAnalyzer request_analyzer(&input);
  request_analyzer.Init();
  for (int i = 0, num = input.num_resources(); i < num; ++i) {
    const Resource& resource = input.GetResource(i);
    // Check the nesting level of each resource and add the dependency trace
    // to the result if it exceeds the thresholds.
    DependencyTrace trace = request_analyzer.GetDependencyTrace(resource);
    if (trace.size() >= kMinNestingLevel) {
      Result* result = provider->NewResult();
      pagespeed::Savings* savings = result->mutable_savings();
      savings->set_critical_path_length_saved(1);

      for (DependencyTrace::const_iterator iter = trace.begin();
          iter != trace.end(); ++iter) {
        result->add_resource_urls(*iter);
      }
    }
  }
  return true;
}

void AvoidExcessSerialization::FormatResults(const ResultVector& results,
                                             RuleFormatter* formatter) {
  for (ResultVector::const_iterator iter = results.begin();
       iter != results.end(); ++iter) {

    UrlBlockFormatter* body = formatter->AddUrlBlock(
        // TRANSLATOR: Header at the top of a list of URLs when Page Speed
        // detected that a resource depends on all the resources shown in the
        // list.
        _("The following requests are serialized. Try to break up the "
          "dependencies to make them load in parallel."));

    const Result& result = **iter;
    for (int i = 0; i < result.resource_urls_size(); i++) {
      body->AddUrl(result.resource_urls(i));
    }
  }
}

bool AvoidExcessSerialization::IsExperimental() const {
  // TODO(michschn): Before graduating from experimental:
  // 1. write unit tests!
  // 2. implement ComputeScore
  // 3. implement ComputeResultImpact
  return true;
}

}  // namespace rules

}  // namespace pagespeed
