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
// CallGraphProfile contains all of the state for a profile
// run. CallGraphProfile is not thread-safe.

#ifndef FUNCTION_CALL_PROFILE_H_
#define FUNCTION_CALL_PROFILE_H_

#include "base/basictypes.h"
#include "base/scoped_ptr.h"

class CallGraphProfileTest;

namespace activity {

class CallGraph;
class CallGraphMetadata;
class CallGraphProfileSnapshot;
class ClockInterface;
class FunctionInfoInterface;
class OutputStreamInterface;
class Profile;
class Timer;

// See comment at top of file for a complete description
class CallGraphProfile {
 public:
  explicit CallGraphProfile(ClockInterface *clock);
  ~CallGraphProfile();

  // Start a profiling session, using the current time for the start
  // time.
  void Start();

  // Start a profiling session, using the specified time for the start
  // time.
  void Start(int64 start_time_usec);

  // Stop the current profiling session.
  void Stop();

  // Are we currently profiling?
  bool profiling() const { return profiling_; }

  // Called when a function has just been invoked.
  void OnFunctionEntry(FunctionInfoInterface *function_info);

  // Called when a function has just returned.
  void OnFunctionExit(FunctionInfoInterface *function_info);

  // Called when a function is first instantiated (e.g. when it is
  // parsed, eval'd, or constructed).
  void OnFunctionInstantiated(FunctionInfoInterface *function_info);

  // Serialize the profiling state to the specified output stream.
  bool SerializeToOutputStream(OutputStreamInterface *out) const;

  // Should a script from the given URL be included in the profile?
  static bool ShouldIncludeInProfile(const char *file_name);

  const Profile *profile() const { return profile_.get(); }
  const CallGraph *call_graph() const { return call_graph_.get(); }
  const CallGraphMetadata *metadata() const { return metadata_.get(); }

  // Create a thread-safe read-only view of the CallGraph and
  // CallGraphMetadata. Ownership of the returned instance is
  // transferred to the caller.
  CallGraphProfileSnapshot *CreateSnapshot() const;

 private:
  scoped_ptr<Profile> profile_;
  scoped_ptr<CallGraph> call_graph_;
  scoped_ptr<CallGraphMetadata> metadata_;
  scoped_ptr<Timer> timer_;
  ClockInterface *const clock_;
  bool profiling_;

  DISALLOW_COPY_AND_ASSIGN(CallGraphProfile);
};

}  // namespace activity

#endif  // FUNCTION_CALL_PROFILE_H_
