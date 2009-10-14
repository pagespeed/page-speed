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
// Various nsIRunnable implementations used to execute code across
// threads.

#ifndef PROFILER_RUNNABLES_H_
#define PROFILER_RUNNABLES_H_

#include <vector>

#include "base/scoped_ptr.h"

#include "nsCOMPtr.h"
#include "nsIRunnable.h"

class IActivityProfilerEvent;
class IActivityProfilerTimelineEventCallback;
class nsIThread;

namespace activity {

class CallGraphProfileSnapshot;

/**
 * GetTimelineEventsRunnable is instantiated in the main thread, but
 * runs in the background thread. GetTimelineEventsRunnable walks the
 * call graph to build a set of ProfilerEvents, and then passes those
 * events to the InvokeTimelineEventsCallbackRunnable, which is
 * invoked on the main thread.
 */
class GetTimelineEventsRunnable : public nsIRunnable {
 private:
  typedef std::vector<nsCOMPtr<IActivityProfilerEvent> > EventVector;

 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE

  GetTimelineEventsRunnable(
    nsIThread *main_thread,
    IActivityProfilerTimelineEventCallback *callback,
    CallGraphProfileSnapshot *snapshot,
    PRInt64 start_time_usec,
    PRInt64 end_time_usec,
    PRInt64 resolution_usec);

 private:
  ~GetTimelineEventsRunnable();

  /**
   * Iterate over the CallGraphProfileSnapshot to build an array of
   * IActivityProfilerEvents that we'll pass up to the UI via the
   * IActivityProfilerTimelineEventCallback.
   */
  nsresult PopulateEventArray(EventVector *events);

  nsCOMPtr<nsIThread> main_thread_;
  nsCOMPtr<IActivityProfilerTimelineEventCallback> callback_;
  scoped_ptr<CallGraphProfileSnapshot> snapshot_;
  PRInt64 start_time_usec_;
  PRInt64 end_time_usec_;
  PRInt64 resolution_usec_;
};

/**
 * InvokeTimelineEventsCallbackRunnable is instantiated in a
 * background thread, but runs on the main
 * thread. InvokeTimelineEventsCallbackRunnable invokes the
 * IActivityProfilerTimelineEventCallback with the vector of ProfilerEvents.
 */
class InvokeTimelineEventsCallbackRunnable : public nsIRunnable {
 private:
  typedef std::vector<nsCOMPtr<IActivityProfilerEvent> > EventVector;

 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE

  InvokeTimelineEventsCallbackRunnable(
      IActivityProfilerTimelineEventCallback *callback,
      EventVector *events);

 private:
  ~InvokeTimelineEventsCallbackRunnable();

  nsCOMPtr<IActivityProfilerTimelineEventCallback> callback_;
  scoped_ptr<EventVector> events_;
};

}  // namespace activity

#endif  // PROFILER_RUNNABLES_H_
