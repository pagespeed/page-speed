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
// ProfilerEvent is the XPCOM equivalent of the CallGraphTimelineEvent
// structure. ProfilerEvent is meant to be passed across the XPCOM
// interface (e.g. from native code to JavaScript).

#ifndef PROFILER_EVENT_H_
#define PROFILER_EVENT_H_

#include "IActivityProfiler.h"
#include "base/basictypes.h"

class nsCStringContainer;

namespace activity {

class ProfilerEvent : public IActivityProfilerEvent {
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_IACTIVITYPROFILEREVENT

  ProfilerEvent(int64 start_time_usec,
              int64 duration_usec,
              int64 intensity,
              int16 type,
              const char *identifier);

 private:
  ~ProfilerEvent();

  PRInt64 start_time_usec_;
  PRInt64 duration_usec_;
  PRInt64 intensity_;
  PRInt16 type_;
  const nsCStringContainer *const identifier_;
};

}  // namespace activity

#endif  // PROFILER_EVENT_H_
