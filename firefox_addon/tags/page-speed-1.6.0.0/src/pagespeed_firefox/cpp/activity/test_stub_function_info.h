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
// Implementation of FunctionInfoInterface for use in unit tests.

#ifndef TEST_STUB_FUNCTION_INFO_H_
#define TEST_STUB_FUNCTION_INFO_H_

#include "base/basictypes.h"
#include "function_info_interface.h"

namespace activity_testing {

class TestStubFunctionInfo : public activity::FunctionInfoInterface {
 public:
  explicit TestStubFunctionInfo(int32 tag)
      : tag_(tag),
        file_name_(""),
        function_name_(""),
        function_source_utf8_("") {
  }

  TestStubFunctionInfo(
      int32 tag,
      const char *file_name,
      const char *function_name,
      const char *function_source_utf8)
      : tag_(tag),
        file_name_(file_name),
        function_name_(function_name),
        function_source_utf8_(function_source_utf8) {
  }

  virtual ~TestStubFunctionInfo() {}
  virtual int32 GetFunctionTag() { return tag_; }
  virtual const char *GetFileName() { return file_name_; }
  virtual const char *GetFunctionName() { return function_name_; }
  virtual const char *GetFunctionSourceUtf8() { return function_source_utf8_; }

 private:
  int32 tag_;
  const char *file_name_;
  const char *function_name_;
  const char *function_source_utf8_;

  DISALLOW_COPY_AND_ASSIGN(TestStubFunctionInfo);
};

}  // namespace activity_testing

#endif  // TEST_STUB_FUNCTION_INFO_H_
