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

#include "pagespeed/testing/instrumentation_data_builder.h"

#include "base/logging.h"
#include "base/stl_util-inl.h"

namespace pagespeed_testing {

InstrumentationDataBuilder::InstrumentationDataBuilder()
    : current_time_(0) {
}

void InstrumentationDataBuilder::Push(
    pagespeed::InstrumentationData_RecordType type) {
  if (root_ == NULL) {
    root_.reset(new pagespeed::InstrumentationData());
    working_set_.push_back(root_.get());
  } else {
    // If this check fires, it means you popped the entire stack of
    // InstrumentationData and then attempted to replace it with a new
    // InstrumentationData. This is an error. Once the stack is
    // popped, you need to take ownership of the InstrumentationData
    // by calling Get() at which point you can reuse the
    // InstrumentationDataBuilder to create another
    // InstrumentationData event tree.
    CHECK(!working_set_.empty())
        << "Unable to add new event to empty working set.";
    working_set_.push_back(Current()->add_children());
  }
  Current()->set_type(type);
  Current()->set_start_time(current_time_++);
}

InstrumentationDataBuilder& InstrumentationDataBuilder::Pop() {
  Current()->set_end_time(current_time_++);
  working_set_.pop_back();
  return *this;
}

pagespeed::InstrumentationData* InstrumentationDataBuilder::Current() {
  CHECK(!working_set_.empty());
  return working_set_.back();
}

pagespeed::InstrumentationData* InstrumentationDataBuilder::Get() {
  Unwind();
  current_time_ = 0;
  return root_.release();
}

void InstrumentationDataBuilder::Unwind() {
  while (!working_set_.empty()) {
    Pop();
  }
}

InstrumentationDataBuilder& InstrumentationDataBuilder::Layout() {
  Push(pagespeed::InstrumentationData_RecordType_LAYOUT);
  return *this;
}

InstrumentationDataBuilder& InstrumentationDataBuilder::EvaluateScript(
    const char* url, int line_number) {
  Push(pagespeed::InstrumentationData_RecordType_EVALUATE_SCRIPT);
  Current()->mutable_data()->set_url(url);
  Current()->mutable_data()->set_line_number(line_number);
  return *this;
}

InstrumentationDataBuilder& InstrumentationDataBuilder::AddFrame(
    const char* url, int line_number, const char* function_name) {
  pagespeed::StackFrame* frame = Current()->add_stack_trace();
  frame->set_url(url);
  frame->set_line_number(line_number);
  frame->set_function_name(function_name);
  return *this;
}

}  // namespace pagespeed_testing

