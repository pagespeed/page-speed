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
// CallGraphTimelineEventSet stores a collection of unique
// CallGraphTimelineEvent objects in a manner that allows for
// efficient lookup.

#ifndef CALL_GRAPH_TIMELINE_EVENT_SET_H_
#define CALL_GRAPH_TIMELINE_EVENT_SET_H_

#include <stdint.h>    // for int64_t

#include <functional>  // for binary_function
#include <map>
#include <utility>     // for pair

#include "call_graph_timeline_event.h"
#include "base/basictypes.h"

namespace activity {

class CallGraphTimelineEventSet {
 private:
  typedef std::pair<CallGraphTimelineEvent::Type, const char *> TypeIdPair;
  typedef std::pair<int64_t, TypeIdPair> Key;

  // STL StrictWeakOrdering implementation used to sort Keys by their
  // start times.
  struct KeyLessThan
      : public std::binary_function<const Key&, const Key&, bool> {
    // Return true if a is less than b.
    bool operator()(const Key &a, const Key &b) const;
  };

 public:
  typedef std::map<Key, CallGraphTimelineEvent*, KeyLessThan> EventMap;

  explicit CallGraphTimelineEventSet(int64_t event_duration_usec);
  ~CallGraphTimelineEventSet();

  /**
   * Gets an existing event with the specified attributes, or
   * constructs a new event if no event with the specified attributes
   * exists already. If a new event is constructed, it is inserted
   * into the map, so subsequent calls to GetOrCreateEvent() with the
   * same parameters will return the same instance.
   */
  CallGraphTimelineEvent *GetOrCreateEvent(
      const char *identifier,
      CallGraphTimelineEvent::Type type,
      int64_t start_time_usec);

  int64_t event_duration_usec() const { return event_duration_usec_; }

  const EventMap *event_map() const { return &event_map_; }

 private:
  EventMap event_map_;
  const int64_t event_duration_usec_;

  DISALLOW_COPY_AND_ASSIGN(CallGraphTimelineEventSet);
};

}  // namespace activity

#endif  // CALL_GRAPH_TIMELINE_EVENT_SET_H_
