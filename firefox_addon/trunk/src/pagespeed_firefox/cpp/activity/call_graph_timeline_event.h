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
// CallGraphTimelineEvent is a simple structure that contains the
// information about what happened within a specified window of time
// (as specified by start_time_usec and duration_usec). This includes
// the amount of code executed, as well as the number of functions
// initialized.

#ifndef CALL_GRAPH_TIMELINE_EVENT_H_
#define CALL_GRAPH_TIMELINE_EVENT_H_

#include "base/basictypes.h"

namespace activity {

struct CallGraphTimelineEvent {
  /** Possible event types. */
  enum Type {
    JS_PARSE = 1,
    JS_EXECUTE
  };

  CallGraphTimelineEvent(
      int64 start_time_usec,
      int64 duration_usec,
      Type type,
      const char *identifier)
      : start_time_usec(start_time_usec),
        duration_usec(duration_usec),
        intensity(0LL),
        type(type),
        identifier(identifier) {
  }

  /** Start time of the event. */
  const int64 start_time_usec;

  /** Duration of the event. */
  const int64 duration_usec;

  /**
   * The intensity of the event. The range of the intensity is event
   * type-dependent.
   */
  int64 intensity;

  /** The type of the event. */
  Type type;

  /**
   * The identifier for this event (e.g. the URL of the file
   * associated with this event).
   */
  const char *const identifier;
};

}  // namespace activity

#endif  // CALL_GRAPH_TIMELINE_EVENT_H_
