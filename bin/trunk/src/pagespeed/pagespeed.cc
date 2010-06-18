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

#include <string>
#include <vector>

#include "base/logging.h"
#include "base/stl_util-inl.h"
#include "googleurl/src/gurl.h"
#include "pagespeed/pagespeed_input_populator.h"
#include "pagespeed/test_shell_runner.h"
#include "third_party/libpagespeed/src/pagespeed/core/engine.h"
#include "third_party/libpagespeed/src/pagespeed/core/pagespeed_input.h"
#include "third_party/libpagespeed/src/pagespeed/core/rule.h"
#include "third_party/libpagespeed/src/pagespeed/formatters/text_formatter.h"
#include "third_party/libpagespeed/src/pagespeed/image_compression/image_attributes_factory.h"
#include "third_party/libpagespeed/src/pagespeed/rules/rule_provider.h"

namespace {

// 2 minutes
const int kTimeoutMillis = 2 * 60 * 1000;

// Loads the web page at the given URL, and returns a PagespeedInput
// instance that's populated with the resources fetched during that
// page load.
pagespeed::PagespeedInput* PopulatePageSpeedInput(const std::string& url) {
  pagespeed::TestShellRunner runner;
  pagespeed::PagespeedInputPopulator populator;

  populator.Attach();
  if (!runner.Run(url, kTimeoutMillis)) {
    return NULL;
  }
  return populator.Detach();
}

bool RunPagespeed(const char* url) {
  const GURL gurl(url);
  if (!gurl.is_valid()) {
    fprintf(stderr, "Invalid URL %s.\n", url);
    return false;
  }

  scoped_ptr<pagespeed::PagespeedInput> input(
      PopulatePageSpeedInput(gurl.spec()));
  if (input == NULL || input->num_resources() == 0) {
    fprintf(stderr,
            "Unable to construct PagespeedInput for %s.\n", url);
    return false;
  }

  input->SetPrimaryResourceUrl(gurl.spec());

  input->AcquireImageAttributesFactory(
      new pagespeed::image_compression::ImageAttributesFactory());

  std::vector<pagespeed::Rule*> rules;

  // In environments where exceptions can be thrown, use
  // STLElementDeleter to make sure we free the rules in the event
  // that they are not transferred to the Engine.
  STLElementDeleter<std::vector<pagespeed::Rule*> > rule_deleter(&rules);

  bool save_optimized_content = false;
  pagespeed::rule_provider::AppendCoreRules(save_optimized_content, &rules);

  // Ownership of rules is transferred to the Engine instance.
  pagespeed::Engine engine(&rules);
  engine.Init();

  scoped_ptr<pagespeed::RuleFormatter> formatter(
      new pagespeed::formatters::TextFormatter(&std::cout));

  engine.ComputeAndFormatResults(*input.get(), formatter.get());

  return true;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <url>\n\n", argv[0]);
    return 1;
  }

  // Only display WARNING and above on the console.
  logging::SetMinLogLevel(logging::LOG_WARNING);

  pagespeed::TestShellRunner::SetUp();
  bool result = RunPagespeed(argv[1]);
  pagespeed::TestShellRunner::TearDown();

  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
