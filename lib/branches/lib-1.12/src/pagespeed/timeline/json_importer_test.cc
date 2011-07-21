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
#include "pagespeed/proto/timeline.pb.h"
#include "pagespeed/testing/pagespeed_test.h"
#include "pagespeed/timeline/json_importer.h"

using pagespeed::InstrumentationData;

namespace {

using pagespeed::timeline::CreateTimelineProtoFromJsonString;

const std::string kTimelineJson =
    "[{"
    "  \"startTime\":1305844473655.642,"
    "  \"endTime\":1305844473655.873,"
    "  \"type\":\"RecalculateStyles\","
    "  \"usedHeapSize\":3114208,"
    "  \"totalHeapSize\":5650432"
    "},{"
    "  \"startTime\":1305844473656.029,"
    "  \"data\":{"
    "    \"type\":\"mousedown\""
    "  },"
    "  \"endTime\":1305844473656.055,"
    "  \"type\":\"EventDispatch\","
    "  \"usedHeapSize\":3114208,"
    "  \"totalHeapSize\":5650432"
    "},{"
    "  \"type\":\"EvaluateScript\","
    "  \"data\":{"
    "    \"url\":\"http://example.com/reflow.html\","
    "    \"lineNumber\":4"
    "  },"
    "  \"children\":[{"
    "    \"type\":\"RecalculateStyles\","
    "    \"stackTrace\":[{"
    "      \"functionName\":\"triggerReflow\","
    "      \"url\":\"http://example.com/reflow.html\","
    "      \"lineNumber\":31,"
    "      \"columnNumber\":30"
    "    },{"
    "      \"functionName\":\"\","
    "      \"url\":\"http://example.com/reflow.html\","
    "      \"lineNumber\":1,"
    "      \"columnNumber\":1"
    "    }]"
    "  },{"
    "    \"type\":\"Layout\","
    "    \"stackTrace\":[{"
    "      \"functionName\":\"triggerReflow\","
    "      \"url\":\"http://example.com/reflow.html\","
    "      \"lineNumber\":31,"
    "      \"columnNumber\":30"
    "    },{"
    "      \"functionName\":\"\","
    "      \"url\":\"http://example.com/reflow.html\","
    "      \"lineNumber\":1,"
    "      \"columnNumber\":1"
    "    }],"
    "  }]"
    "}]";

TEST(TimelineTest, Basic) {
  std::vector<const InstrumentationData*> records;
  STLElementDeleter<std::vector<const InstrumentationData*> > deleter(&records);
  ASSERT_TRUE(CreateTimelineProtoFromJsonString(kTimelineJson, &records));
  ASSERT_EQ(3u, records.size());

  const InstrumentationData& record0 = *records[0];
  EXPECT_EQ(InstrumentationData::RECALCULATE_STYLES, record0.type());
  EXPECT_EQ(1305844473655.642, record0.start_time());
  EXPECT_EQ(1305844473655.873, record0.end_time());
  EXPECT_EQ(3114208, record0.used_heap_size());
  EXPECT_EQ(5650432, record0.total_heap_size());
  EXPECT_EQ(0, record0.children_size());
  EXPECT_FALSE(record0.has_data());

  const InstrumentationData& record1 = *records[1];
  EXPECT_EQ(InstrumentationData::EVENT_DISPATCH, record1.type());
  EXPECT_EQ(1305844473656.029, record1.start_time());
  EXPECT_EQ(1305844473656.055, record1.end_time());
  EXPECT_TRUE(record1.has_data());
  EXPECT_EQ("mousedown", record1.data().type());

  const InstrumentationData& record2 = *records[2];
  EXPECT_EQ(InstrumentationData::EVALUATE_SCRIPT, record2.type());
  EXPECT_EQ("http://example.com/reflow.html", record2.data().url());
  EXPECT_EQ(4, record2.data().line_number());
  EXPECT_FALSE(record2.data().has_type());
  ASSERT_EQ(2, record2.children_size());

  const InstrumentationData& record2a = record2.children(0);
  EXPECT_EQ(InstrumentationData::RECALCULATE_STYLES, record2a.type());
  ASSERT_EQ(2, record2a.stack_trace_size());

  const pagespeed::StackFrame& frame2a0 = record2a.stack_trace(0);
  EXPECT_EQ("triggerReflow", frame2a0.function_name());
  EXPECT_EQ("http://example.com/reflow.html", frame2a0.url());
  EXPECT_EQ(31, frame2a0.line_number());
  EXPECT_EQ(30, frame2a0.column_number());

  const pagespeed::StackFrame& frame2a1 = record2a.stack_trace(1);
  EXPECT_EQ("", frame2a1.function_name());
  EXPECT_EQ("http://example.com/reflow.html", frame2a1.url());
  EXPECT_EQ(1, frame2a1.line_number());
  EXPECT_EQ(1, frame2a1.column_number());

  const InstrumentationData& record2b = record2.children(1);
  EXPECT_EQ(InstrumentationData::LAYOUT, record2b.type());
  ASSERT_EQ(2, record2b.stack_trace_size());
}

}  // namespace
