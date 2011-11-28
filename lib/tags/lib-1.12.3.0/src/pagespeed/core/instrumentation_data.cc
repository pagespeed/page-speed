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

#include "pagespeed/core/instrumentation_data.h"
#include "pagespeed/proto/timeline.pb.h"

namespace pagespeed {

InstrumentationDataVisitor::InstrumentationDataVisitor() {}
InstrumentationDataVisitor::~InstrumentationDataVisitor() {}

// static
void InstrumentationDataVisitor::Traverse(
    InstrumentationDataVisitor* visitor,
    const InstrumentationDataStack& data) {
  for (InstrumentationDataStack::const_iterator
           it = data.begin(), end = data.end(); it != end; ++it) {
    Traverse(visitor, **it);
  }
}

// static
void InstrumentationDataVisitor::Traverse(InstrumentationDataVisitor* visitor,
                                          const InstrumentationData& data) {
  InstrumentationDataStack stack;
  stack.push_back(&data);
  TraverseImpl(visitor, &stack);
  stack.pop_back();
}

// static
void InstrumentationDataVisitor::TraverseImpl(
    InstrumentationDataVisitor* visitor,
    InstrumentationDataStack* stack) {
  const InstrumentationData& data = *stack->back();
  if (visitor->Visit(*stack)) {
    for (int i = 0; i < data.children_size(); ++i) {
      stack->push_back(&data.children(i));
      TraverseImpl(visitor, stack);
      stack->pop_back();
    }
  }
}

}  // namespace pagespeed
