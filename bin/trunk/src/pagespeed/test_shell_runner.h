// Copyright 2010 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef PAGESPEED_PAGESPEED_TEST_SHELL_RUNNER_H_
#define PAGESPEED_PAGESPEED_TEST_SHELL_RUNNER_H_

#include <string>

namespace pagespeed {

// TestShellRunner is a wrapper around TestShell that abstracts away
// the webkit dependencies, so we can use the TestShell without having to
// directly include things like the DumpRenderTree config.h, which does
// various nasty things like hiding base/logging.h.
class TestShellRunner {
public:
  // SetUp and TearDown the TestShellRunner environment. Should be
  // called at program startup and shutdown (e.g. in main()).
  static void SetUp();
  static void TearDown();

  // Load the web page at the given URL, with the specified
  // timeout. If the timeout is exceeded then execution will be
  // aborted.
  bool Run(const std::string& url, int timeout_millis);
};

}  // namespace pagespeed

#endif  // PAGESPEED_PAGESPEED_TEST_SHELL_RUNNER_H_

