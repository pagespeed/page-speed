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

#include "profiler_event.h"

#include "nsStringAPI.h"

NS_IMPL_THREADSAFE_ISUPPORTS1(activity::ProfilerEvent, IActivityProfilerEvent)

namespace activity {

ProfilerEvent::ProfilerEvent(int64 start_time_usec,
                         int64 duration_usec,
                         int64 intensity,
                         int16_t type,
                         const char *identifier)
    : start_time_usec_(start_time_usec),
      duration_usec_(duration_usec),
      intensity_(intensity),
      type_(type),
      identifier_(new nsCString(identifier)) {
}

ProfilerEvent::~ProfilerEvent() {
  delete identifier_;
}

NS_IMETHODIMP ProfilerEvent::GetIdentifier(char **retval) {
  if (NULL == retval) {
    return NS_ERROR_NULL_POINTER;
  }
  *retval = NS_CStringCloneData(*identifier_);
  return NS_OK;
}

NS_IMETHODIMP ProfilerEvent::GetStartTimeUsec(PRInt64 *retval) {
  if (NULL == retval) {
    return NS_ERROR_NULL_POINTER;
  }
  *retval = start_time_usec_;
  return NS_OK;
}

NS_IMETHODIMP ProfilerEvent::GetDurationUsec(PRInt64 *retval) {
  if (NULL == retval) {
    return NS_ERROR_NULL_POINTER;
  }
  *retval = duration_usec_;
  return NS_OK;
}

NS_IMETHODIMP ProfilerEvent::GetIntensity(PRInt64 *retval) {
  if (NULL == retval) {
    return NS_ERROR_NULL_POINTER;
  }
  *retval = intensity_;
  return NS_OK;
}

NS_IMETHODIMP ProfilerEvent::GetType(PRInt16 *retval) {
  if (NULL == retval) {
    return NS_ERROR_NULL_POINTER;
  }
  *retval = type_;
  return NS_OK;
}

}  // namespace activity
