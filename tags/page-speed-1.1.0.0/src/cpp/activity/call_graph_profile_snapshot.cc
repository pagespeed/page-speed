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

#include "call_graph_profile_snapshot.h"

#include <utility>  // for make_pair

#include "call_graph.h"
#include "call_graph_metadata.h"
#include "call_graph_profile.h"
#include "profile.pb.h"
#include "check.h"

namespace activity {

CallGraphProfileSnapshot::CallGraphProfileSnapshot(
      const CallGraphProfile &profile,
      CallGraph *call_graph,
      CallGraphMetadata *metadata)
    : profile_(profile),
      call_graph_(call_graph),
      metadata_(metadata) {
}

CallGraphProfileSnapshot::~CallGraphProfileSnapshot() {}

void CallGraphProfileSnapshot::Init(
    int64_t start_time_usec, int64_t end_time_usec) {
  PopulateInitTimeMap(start_time_usec, end_time_usec);
}

void CallGraphProfileSnapshot::PopulateInitTimeMap(
    int64_t start_time_usec, int64_t end_time_usec) {
  GCHECK_EQ(0, init_time_map_.size());
  GCHECK_GE(start_time_usec, 0);
  GCHECK_GE(end_time_usec, 0);
  GCHECK_GE(end_time_usec, start_time_usec);

  for (CallGraphMetadata::MetadataMap::const_iterator
           it = metadata_->map()->begin(),
           end = metadata_->map()->end();
       it != end;
       ++it) {
    const FunctionMetadata &function_metadata = *it->second;
    if (function_metadata.has_function_instantiation_time_usec()) {
      int64_t function_instantiation_time_usec =
          function_metadata.function_instantiation_time_usec();
      if (function_instantiation_time_usec >= start_time_usec &&
          function_instantiation_time_usec <= end_time_usec) {
        // The function has an init time, and it is within the range
        // of time we're interested in. Add it to the map.
        init_time_map_.insert(std::make_pair(
            function_instantiation_time_usec, &function_metadata));
      }
    }
  }
}

}  // namespace activity
