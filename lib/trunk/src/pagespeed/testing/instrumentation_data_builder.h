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

#ifndef PAGESPEED_TESTING_INSTRUMENTATION_DATA_BUILDER_H_
#define PAGESPEED_TESTING_INSTRUMENTATION_DATA_BUILDER_H_

#include <vector>

#include "base/basictypes.h"
#include "base/scoped_ptr.h"
#include "pagespeed/proto/timeline.pb.h"

namespace pagespeed_testing {

// Builder for InstrumentationData instances. See the unit test for
// example usage.
class InstrumentationDataBuilder {
 public:
  InstrumentationDataBuilder();

  // Methods to construct a new InstrumentationData instance of the
  // specified type. Add other event types as they are needed.
  InstrumentationDataBuilder& Layout();
  InstrumentationDataBuilder& EvaluateScript(const char* url, int line_number);

  // Pop to the parent InstrumentationData.
  InstrumentationDataBuilder& Pop();

  // Get the built InstrumentationData instance. Ownership of the
  // InstrumentationData is transferred to the caller and the internal
  // state of this object is reset, so it can be reused.
  pagespeed::InstrumentationData* Get();

  // Add a new stack frame to the current InstrumentationData instance.
  InstrumentationDataBuilder& AddFrame(
      const char* url, int line_number, const char* function_name);

 private:
  void Push(pagespeed::InstrumentationData_RecordType type);
  pagespeed::InstrumentationData* Current();
  void Unwind();

  scoped_ptr<pagespeed::InstrumentationData> root_;
  std::vector<pagespeed::InstrumentationData*> working_set_;
  int current_time_;

  DISALLOW_COPY_AND_ASSIGN(InstrumentationDataBuilder);
};

}  // namespace pagespeed_testing

#endif  // PAGESPEED_TESTING_INSTRUMENTATION_DATA_BUILDER_H_

