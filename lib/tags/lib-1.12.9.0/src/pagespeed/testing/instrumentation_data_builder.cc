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
    : current_time_(0.0), current_tick_(0) {
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
  Current()->set_start_time(current_time_);
  Current()->set_start_tick(current_tick_);
  current_time_ += 1.0;
  ++current_tick_;
}

InstrumentationDataBuilder& InstrumentationDataBuilder::Pop() {
  Current()->set_end_time(current_time_);
  Current()->set_end_tick(current_tick_);
  current_time_ += 1.0;
  ++current_tick_;
  working_set_.pop_back();
  return *this;
}

pagespeed::InstrumentationData* InstrumentationDataBuilder::Current() {
  CHECK(!working_set_.empty());
  return working_set_.back();
}

pagespeed::InstrumentationData* InstrumentationDataBuilder::Get() {
  Unwind();
  return root_.release();
}

void InstrumentationDataBuilder::Unwind() {
  while (!working_set_.empty()) {
    Pop();
  }
}

InstrumentationDataBuilder& InstrumentationDataBuilder::EvaluateScript(
    const char* url, int line_number) {
  Push(pagespeed::InstrumentationData::EVALUATE_SCRIPT);
  Current()->mutable_data()->set_url(url);
  Current()->mutable_data()->set_line_number(line_number);
  return *this;
}

InstrumentationDataBuilder& InstrumentationDataBuilder::FunctionCall(
    const char* script_name, int script_line) {
  Push(pagespeed::InstrumentationData::FUNCTION_CALL);
  Current()->mutable_data()->set_script_name(script_name);
  Current()->mutable_data()->set_script_line(script_line);
  return *this;
}

InstrumentationDataBuilder& InstrumentationDataBuilder::Layout() {
  Push(pagespeed::InstrumentationData::LAYOUT);
  return *this;
}

InstrumentationDataBuilder& InstrumentationDataBuilder::ParseHTML(
    int length, int start_line, int end_line) {
  Push(pagespeed::InstrumentationData::PARSE_HTML);
  Current()->mutable_data()->set_length(length);
  Current()->mutable_data()->set_start_line(start_line);
  Current()->mutable_data()->set_end_line(end_line);
  return *this;
}

InstrumentationDataBuilder& InstrumentationDataBuilder::TimerInstall(
    int timer_id, bool single_shot, int timeout) {
  Push(pagespeed::InstrumentationData::TIMER_INSTALL);
  Current()->mutable_data()->set_timer_id(timer_id);
  Current()->mutable_data()->set_single_shot(single_shot);
  Current()->mutable_data()->set_timeout(timeout);
  return *this;
}

InstrumentationDataBuilder& InstrumentationDataBuilder::TimerFire(int timer_id) {
  Push(pagespeed::InstrumentationData::TIMER_FIRE);
  Current()->mutable_data()->set_timer_id(timer_id);
  return *this;
}

InstrumentationDataBuilder& InstrumentationDataBuilder::Pause(double millis) {
  current_time_ += millis;
  return *this;
}

InstrumentationDataBuilder& InstrumentationDataBuilder::AddFrame(
    const char* url,
    int line_number,
    int column_number,
    const char* function_name) {
  pagespeed::StackFrame* frame = Current()->add_stack_trace();
  frame->set_url(url);
  frame->set_line_number(line_number);
  frame->set_column_number(column_number);
  frame->set_function_name(function_name);
  return *this;
}

}  // namespace pagespeed_testing
