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

#include <string>
#include <vector>

#include "base/scoped_ptr.h"
#include "base/stl_util-inl.h"
#include "pagespeed/core/instrumentation_data.h"
#include "pagespeed/proto/timeline.pb.h"
#include "pagespeed/testing/instrumentation_data_builder.h"
#include "pagespeed/testing/pagespeed_test.h"

using pagespeed::InstrumentationData;
using pagespeed::InstrumentationDataVector;
using pagespeed::InstrumentationDataVisitor;
using pagespeed_testing::AssertProtoEq;
using pagespeed_testing::InstrumentationDataBuilder;

namespace {

// Simple visitor implementation that copies each node into a
// destination vector of nodes. Used to verify correctness of the
// visitor traversal implementation.
class CopyVisitor : public pagespeed::InstrumentationDataVisitor {
 public:
  // data is the vector that copied nodes should be stored in. The
  // caller is responsible for freeing the nodes in the vector.
  CopyVisitor(std::vector<const InstrumentationData*>* data) : data_(data) {}
  virtual bool Visit(const std::vector<const InstrumentationData*>& stack);

 private:
  std::vector<const InstrumentationData*>* data_;
  std::vector<InstrumentationData*> working_set_;
};

bool CopyVisitor::Visit(const std::vector<const InstrumentationData*>& stack) {
  if (stack.size() <= working_set_.size()) {
    // If the passed in stack is more shallow than our stack, it
    // indicates that we've traversed up at least one parent node
    // since the last invocation. Thus we need to trim our stack to
    // match.
    working_set_.resize(stack.size());

    // Trim one more node from our working set, in order to make room
    // for the newly visited node.
    working_set_.pop_back();
  }

  InstrumentationData* child;
  if (working_set_.empty()) {
    child = new InstrumentationData();
    data_->push_back(child);
  } else {
    child = working_set_.back()->add_children();
  }
  child->MergeFrom(*stack.back());

  // We visit each child as part of the traversal. We need to manually
  // clear the merged children here so we merge them when they are
  // visited.
  child->clear_children();
  working_set_.push_back(child);
  return true;
}

TEST(InstrumentationDataTest, InstrumentationDataVisitor) {
  InstrumentationDataVector records;
  STLElementDeleter<InstrumentationDataVector> deleter(&records);

  InstrumentationDataBuilder builder;
  records.push_back(builder
                    .ParseHTML(0, 0, 0)
                    .EvaluateScript("http://www.foo.com/", 0)
                    .Layout()
                    .Layout()
                    .AddFrame("http://www.bar.com/", 1, 2, "funcName")
                    .Get());
  records.push_back(builder
                    .EvaluateScript("http://www.foo.com/", 10)
                    .Layout()
                    .AddFrame("http://www.bar.com/", 1, 2, "funcName")
                    .Pop()
                    .Layout()
                    .Get());

  InstrumentationDataVector records_copy;
  STLElementDeleter<InstrumentationDataVector> deleter2(&records_copy);

  CopyVisitor visitor(&records_copy);
  InstrumentationDataVisitor::Traverse(&visitor, records);

  // Verify that the copied records match the original records.
  ASSERT_EQ(records.size(), records_copy.size());
  for (size_t i = 0; i < records.size(); ++i) {
    AssertProtoEq(*records[i], *records_copy[i]);
  }
}

}  // namespace
