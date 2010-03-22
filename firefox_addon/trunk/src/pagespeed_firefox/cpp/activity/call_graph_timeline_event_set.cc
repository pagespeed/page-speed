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

#include "call_graph_timeline_event_set.h"

#include <string.h>        // for strcmp

#include "base/logging.h"
#include "call_graph_timeline_event.h"

namespace activity {

CallGraphTimelineEventSet::CallGraphTimelineEventSet(
    int64 event_duration_usec)
    : event_duration_usec_(event_duration_usec) {
  if (event_duration_usec <= 0LL) {
    LOG(DFATAL) << "Bad event_duration_usec: " << event_duration_usec;
  }
}

CallGraphTimelineEventSet::~CallGraphTimelineEventSet() {
  for (EventMap::const_iterator it = event_map_.begin(),
           end = event_map_.end();
       it != end;
       ++it) {
    delete it->second;
  }
}

CallGraphTimelineEvent *CallGraphTimelineEventSet::GetOrCreateEvent(
    const char *identifier,
    CallGraphTimelineEvent::Type type,
    int64 start_time_usec) {
  if (identifier == NULL) {
    LOG(DFATAL) << "Identifier is null.";
    return NULL;
  }

  if (start_time_usec < 0LL) {
    LOG(DFATAL) << "Bad start_time_usec: " << start_time_usec;
    return NULL;
  }

  Key key(start_time_usec, std::make_pair(type, identifier));
  CallGraphTimelineEvent *event;
  EventMap::iterator it = event_map_.find(key);
  if (it == event_map_.end()) {
    // Construct a new event.
    event = new CallGraphTimelineEvent(
        start_time_usec, event_duration_usec_, type, identifier);
    event_map_[key] = event;
  } else {
    // Use the existing event.
    event = it->second;
  }

  return event;
}

bool CallGraphTimelineEventSet::KeyLessThan::operator()(
    const Key &a, const Key &b) const {
  if (a.first == b.first) {
    const CallGraphTimelineEvent::Type a_type = a.second.first;
    const CallGraphTimelineEvent::Type b_type = b.second.first;
    if (a_type == b_type) {
      // Tie-breaker: if the start times are equal, compare on the
      // identifier url.
      return strcmp(a.second.second, b.second.second) < 0;
    } else {
      return a_type < b_type;
    }
  } else {
    return a.first < b.first;
  }
}

}  // namespace activity
