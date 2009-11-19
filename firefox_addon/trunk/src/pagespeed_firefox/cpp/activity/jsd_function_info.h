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
// JavaScript debugger (JSD) implementation of the
// FunctionInfoInterface.

#ifndef JSD_FUNCTION_INFO_H_
#define JSD_FUNCTION_INFO_H_

#include "base/basictypes.h"
#include "function_info_interface.h"
#include "jsd.h"

namespace activity {

// JsdFunctionInfo lazily fetches the data from the jsdIScript that
// backs it, so each method does some small amount of work the first
// time it is called.
class JsdFunctionInfo : public FunctionInfoInterface {
 public:
  explicit JsdFunctionInfo(jsdIScript *script);
  virtual ~JsdFunctionInfo();

  virtual int32 GetFunctionTag();
  virtual const char *GetFileName();
  virtual const char *GetFunctionName();
  virtual const char *GetFunctionSourceUtf8();

 private:
  jsdIScript *const script_;
  int32 tag_;
  char *file_name_;
  char *function_name_;
  char *function_source_utf8_;

  DISALLOW_COPY_AND_ASSIGN(JsdFunctionInfo);
};

}  // namespace activity

#endif  // JSD_FUNCTION_INFO_H_
