/**
 * Copyright 2008-2009 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Author: Bryan McQuade
//
// CallGraphProfile implementation. See call_graph_profile.h for
// details.

#include "call_graph_profile.h"

#include <string.h>

#include "call_graph.h"
#include "call_graph_metadata.h"
#include "call_graph_profile_snapshot.h"
#include "clock.h"
#include "function_info_interface.h"
#include "profile.pb.h"
#include "check.h"
#include "timer.h"

namespace {

const char *kPrefixFilters[] = { "about:", "chrome:", "file:", "javascript:" };
const char *kSuffixFilters[] = { ".cpp" };
const char *kFullFilters[] = { "XStringBundle" };

const size_t kPrefixFilterSize =
    sizeof(kPrefixFilters) / sizeof(kPrefixFilters[0]);

const size_t kSuffixFilterSize =
    sizeof(kSuffixFilters) / sizeof(kSuffixFilters[0]);

const size_t kFullFilterSize =
    sizeof(kFullFilters) / sizeof(kFullFilters[0]);

bool PrefixMatches(const char *candidate) {
  for (size_t i = 0; i < kPrefixFilterSize; ++i) {
    const char *prefix = kPrefixFilters[i];
    if (strncmp(prefix, candidate, strlen(prefix)) == 0) {
      return true;
    }
  }

  return false;
}

bool SuffixMatches(const char *candidate) {
  const size_t candidate_length = strlen(candidate);
  const char *const candidate_end = candidate + candidate_length;
  for (size_t i = 0; i < kSuffixFilterSize; ++i) {
    const char *suffix = kSuffixFilters[i];
    size_t suffix_length = strlen(suffix);
    if (candidate_length >= suffix_length) {
      const char *candidate_suffix = candidate_end - suffix_length;
      if (strcmp(candidate_suffix, suffix) == 0) {
        return true;
      }
    }
  }

  return false;
}

bool FullMatch(const char *candidate) {
  for (size_t i = 0; i < kFullFilterSize; ++i) {
    if (strcmp(candidate, kFullFilters[i]) == 0) {
      return true;
    }
  }

  return false;
}

}  // namespace

namespace activity {

CallGraphProfile::CallGraphProfile(ClockInterface *clock)
    : clock_(clock),
      profiling_(false) {
}

CallGraphProfile::~CallGraphProfile() {
}

void CallGraphProfile::Start() {
  Start(clock_->GetCurrentTimeUsec());
}

void CallGraphProfile::Start(int64_t start_time_usec) {
  GCHECK(!profiling());
  timer_.reset(new Timer(clock_, start_time_usec));
  profile_.reset(new Profile);
  call_graph_.reset(new CallGraph(profile_.get(), timer_.get()));
  metadata_.reset(new CallGraphMetadata(profile_.get()));
  profile_->set_start_time_usec(start_time_usec);
  profiling_ = true;
}

void CallGraphProfile::Stop() {
  GCHECK(profiling());
  profiling_ = false;
  profile_->set_duration_usec(timer_->GetElapsedTimeUsec());

  if (call_graph_->IsPartiallyConstructed()) {
    // The last call tree was only partially populated by the time
    // Stop() got called, so remove it from the set of call trees.
    profile_->mutable_call_tree()->RemoveLast();
  }
}

void CallGraphProfile::OnFunctionEntry() {
  GCHECK(profiling());
  call_graph_->OnFunctionEntry();
}

void CallGraphProfile::OnFunctionExit(
    FunctionInfoInterface *function_info) {
  GCHECK(profiling());

  const int32_t tag = function_info->GetFunctionTag();

  // If we haven't recorded the metadata for this function already, do
  // so now.
  if (!metadata_->HasEntry(tag)) {
    metadata_->AddEntry(
        tag,
        function_info->GetFileName(),
        function_info->GetFunctionName(),
        function_info->GetFunctionSourceUtf8(),
        -1);
  }

  call_graph_->OnFunctionExit(tag);
}

void CallGraphProfile::OnFunctionInstantiated(
    FunctionInfoInterface *function_info) {
  GCHECK(profiling());

  const int32_t tag = function_info->GetFunctionTag();
  GCHECK(!metadata_->HasEntry(tag));

  const int64_t function_instantiation_time_usec = timer_->GetElapsedTimeUsec();

  metadata_->AddEntry(
      tag,
      function_info->GetFileName(),
      function_info->GetFunctionName(),
      function_info->GetFunctionSourceUtf8(),
      function_instantiation_time_usec);
}

bool CallGraphProfile::SerializeToFileDescriptor(int fd) const {
  GCHECK(!profiling());
  profile_->SerializeToFileDescriptor(fd);
  return true;
}

bool CallGraphProfile::ShouldIncludeInProfile(const char *file_name) const {
  return !PrefixMatches(file_name) &&
      !SuffixMatches(file_name) &&
      !FullMatch(file_name);
}

CallGraphProfileSnapshot *CallGraphProfile::CreateSnapshot() const {
  return new CallGraphProfileSnapshot(
      *this,
      call_graph_->CreateSnapshot(),
      metadata_->CreateSnapshot());
}

}  // namespace activity
