/**
 * Copyright 2009 Google Inc.
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

#include "jsdIDebuggerService_3_6.h"
#include "jsd_wrapper_tmpl.h"

class Jsd3_6Traits {
 public:
  typedef jsdICallHook_3_6 jsdICallHook;
  typedef jsdIDebuggerService_3_6 jsdIDebuggerService;
  typedef jsdIScriptHook_3_6 jsdIScriptHook;
 private:
  Jsd3_6Traits();
};

namespace activity {

JsdWrapper* JsdWrapper::Create_3_6(nsISupports *jsd) {
  return JsdWrapperTmpl<Jsd3_6Traits>::Create(jsd);
}

}  // namespace activity
