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
// CallGraphProfileSnapshot provides a thread-safe read-only snapshot
// of a CallGraphProfile.

#ifndef CALL_GRAPH_PROFILE_SNAPSHOT_H_
#define CALL_GRAPH_PROFILE_SNAPSHOT_H_

#include <map>

#include "base/basictypes.h"
#include "base/scoped_ptr.h"

namespace activity {

class CallGraph;
class CallGraphMetadata;
class CallGraphProfile;
class FunctionMetadata;

class CallGraphProfileSnapshot {
 public:
  typedef std::multimap<int64, const FunctionMetadata*> InitTimeMap;

  ~CallGraphProfileSnapshot();

  const CallGraph *call_graph() const { return call_graph_.get(); }
  const CallGraphMetadata *metadata() const { return metadata_.get(); }

  // Initialize the metadata's init time map and any other structures
  // used by this snapshot.
  void Init(int64 start_time_usec, int64 end_time_usec);

  // A map from function init time to function metadata.
  const InitTimeMap *init_time_map() const { return &init_time_map_; }

 private:
  friend class CallGraphProfile;

  // Only instantiable via CallGraphProfile::CreateSnapshot().
  explicit CallGraphProfileSnapshot(
      const CallGraphProfile &profile,
      CallGraph *call_graph,
      CallGraphMetadata *metadata);

  // Populate the init_time_map_.
  void PopulateInitTimeMap(int64 start_time_usec, int64 end_time_usec);

  // Map from the time a function was initialized to the associated
  // metadata.
  InitTimeMap init_time_map_;

  const CallGraphProfile &profile_;
  scoped_ptr<CallGraph> call_graph_;
  scoped_ptr<CallGraphMetadata> metadata_;

  DISALLOW_COPY_AND_ASSIGN(CallGraphProfileSnapshot);
};

}  // namespace activity

#endif  // CALL_GRAPH_PROFILE_SNAPSHOT_H_
