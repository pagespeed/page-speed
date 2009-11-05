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

// Profiler implements the extension interface defined in
// IActivityProfiler.idl.  A single object of this type is
// instantiated by xpcom when the js portion of the extension requests
// an instance of the activity profiler service.

#ifndef PROFILER_H_
#define PROFILER_H_

#include "IActivityProfiler.h"

#include "base/basictypes.h"
#include "base/scoped_ptr.h"

#include "nsCOMPtr.h"

class nsIThread;

namespace activity {

class CallGraphProfile;
class ClockInterface;
class JsdCallHook;
class JsdScriptHook;

// See comment at top of file for a complete description
class Profiler : public IActivityProfiler {
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_IACTIVITYPROFILER

  Profiler();

 private:
  // Classes that implement XPCom interfaces should have private
  // destructors.  XPCom uses ref counts for memory management; it is
  // illegal to use any other means to do memory management on these
  // objects.
  ~Profiler();

  scoped_ptr<ClockInterface> clock_;
  scoped_ptr<CallGraphProfile> profile_;
  nsCOMPtr<JsdCallHook> call_hook_;
  nsCOMPtr<JsdScriptHook> script_hook_;

  nsCOMPtr<nsIThread> background_thread_;
  nsCOMPtr<nsIThread> main_thread_;

  PRInt16 state_;
  bool error_;

  DISALLOW_COPY_AND_ASSIGN(Profiler);
};

}  // namespace activity

#endif  // PROFILER_H_
