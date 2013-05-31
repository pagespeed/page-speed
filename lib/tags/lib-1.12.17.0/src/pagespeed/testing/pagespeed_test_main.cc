// Copyright 2010 Google Inc. All Rights Reserved.
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

#include <iostream>

#include "google/protobuf/stubs/common.h"
#include "pagespeed/core/pagespeed_init.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/gflags/src/google/gflags.h"

namespace {

// Helper class that will run our exit functions in its destructor.
class ScopedShutDown {
 public:
  ~ScopedShutDown() {
    pagespeed::ShutDown();
    ::google::protobuf::ShutdownProtobufLibrary();
    ::google::ShutDownCommandLineFlags();
  }
};

// We need to declare this as a global variable since gflags and other
// libraries may invoke exit(), so simply declaring a ScopedShutDown
// in main() would not guarantee that shutdown hooks get run (should
// gflags invoke exit() to terminate execution early). This is only
// necessary to make sure that valgrind, etc, don't detect leaks on
// shutdown.
ScopedShutDown g_shutdown;

}  // namespace

int main(int argc, char **argv) {
  std::cout << "Running main() from pagespeed_test_main.cc" << std::endl;

  if (!pagespeed::Init()) {
    std::cerr << "Failed to initialize PageSpeed. Aborting." << std::endl;
    return EXIT_FAILURE;
  }

  testing::InitGoogleTest(&argc, argv);
  ::google::SetUsageMessage("Runner for Page Speed tests.");
  ::google::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
