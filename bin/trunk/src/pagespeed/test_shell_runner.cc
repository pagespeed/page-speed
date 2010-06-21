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

#include "pagespeed/test_shell_runner.h"

#include "third_party/WebKit/WebKitTools/DumpRenderTree/chromium/config.h"
#include "third_party/WebKit/WebKitTools/DumpRenderTree/chromium/TestShell.h"
#include "webkit/support/webkit_support.h"

void platformInit();

namespace pagespeed {

void TestShellRunner::SetUp() {
  webkit_support::SetUpTestEnvironment();
  platformInit();
}

void TestShellRunner::TearDown() {
  webkit_support::TearDownTestEnvironment();
}

bool TestShellRunner::Run(const std::string& url, int timeout_millis) {
  TestParams params;
  params.dumpTree = false;
  params.testUrl = webkit_support::CreateURLForPathOrURL(url);
  if (!params.testUrl.isValid()) {
    return false;
  }

  TestShell shell(false);

  shell.resetTestController();
  shell.setAllowExternalPages(true);
  shell.setLayoutTestTimeout(timeout_millis);
  shell.runFileTest(params);

  // Invoke the JavaScript engine's garbage collector twice, to force
  // a synchronous GC. We do so in order to support checking for
  // memory leaks.
  shell.callJSGC();
  shell.callJSGC();

  // When we finish the last test, cleanup the LayoutTestController.
  // It may have references to not-yet-cleaned up windows.  By
  // cleaning up here we help purify reports.
  shell.resetTestController();

  return true;
}

}  // namespace pagespeed
