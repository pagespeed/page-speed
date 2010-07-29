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

#include "base/command_line.h"
#include "base/logging.h"
#include "base/message_loop_proxy.h"
#include "base/ref_counted.h"
#include "base/stl_util-inl.h"
#include "googleurl/src/gurl.h"
#include "pagespeed/chromium_dom.h"
#include "pagespeed/pagespeed_input_populator.h"
#include "pagespeed/test_shell_runner.h"
#include "third_party/libpagespeed/src/pagespeed/core/engine.h"
#include "third_party/libpagespeed/src/pagespeed/core/pagespeed_input.h"
#include "third_party/libpagespeed/src/pagespeed/core/rule.h"
#include "third_party/libpagespeed/src/pagespeed/formatters/text_formatter.h"
#include "third_party/libpagespeed/src/pagespeed/image_compression/image_attributes_factory.h"
#include "third_party/libpagespeed/src/pagespeed/rules/rule_provider.h"
#include "third_party/WebKit/WebKit/chromium/public/WebDocument.h"
#include "third_party/WebKit/WebKit/chromium/public/WebFrame.h"
#include "webkit/tools/test_shell/test_shell_platform_delegate.h"
#include "webkit/tools/test_shell/simple_resource_loader_bridge.h"

namespace {

// 2 minutes
const int kTimeoutMillis = 2 * 60 * 1000;

void RunEngine(pagespeed::PagespeedInput* input) {
  std::vector<pagespeed::Rule*> rules;

  // In environments where exceptions can be thrown, use
  // STLElementDeleter to make sure we free the rules in the event
  // that they are not transferred to the Engine.
  STLElementDeleter<std::vector<pagespeed::Rule*> > rule_deleter(&rules);

  bool save_optimized_content = false;
  pagespeed::rule_provider::AppendAllRules(save_optimized_content, &rules);

  // Ownership of rules is transferred to the Engine instance.
  pagespeed::Engine engine(&rules);
  engine.Init();

  scoped_ptr<pagespeed::RuleFormatter> formatter(
      new pagespeed::formatters::TextFormatter(&std::cout));

  engine.ComputeAndFormatResults(*input, formatter.get());
}

pagespeed::PagespeedInput* LoadPage(pagespeed::TestShellRunner* runner,
                                    const char* url,
                                    WebKit::WebFrame** out_frame) {
  // Get a handle to the IO thread from the
  // SimpleResourceLoaderBridge, since TestShell uses the
  // SimpleResourceLoaderBridge for its resource loading
  // operations. We need a handle to the IO thread so we can interact
  // with the URLRequestJobTracker, which can only be used from the IO
  // thread.
  scoped_refptr<base::MessageLoopProxy> io_thread_proxy(
      SimpleResourceLoaderBridge::GetIoThread());

  // Instantiate the PagespeedInputPopulator, which observes all HTTP
  // traffic in order to populate a PagespeedInput structure.
  scoped_refptr<pagespeed::PagespeedInputPopulator> populator(
      new pagespeed::PagespeedInputPopulator(io_thread_proxy));
  if (!populator->Attach()) {
    return NULL;
  }

  // Make the TestShellRunner load the page, and get a handle to the
  // WebFrame, which has a reference to the page's DOM.
  if (!runner->Run(url, kTimeoutMillis, out_frame)) {
    return NULL;
  }

  // Return the populated PagespeedInput structure.
  return populator->Detach();
}

bool RunPagespeed(const char* url) {
  const GURL gurl(url);
  if (!gurl.is_valid()) {
    fprintf(stderr, "Invalid URL %s.\n", url);
    return false;
  }

  // The page DOM's lifetime is scoped by the TestShellRunner
  // instance, so we need to make sure that the TestShellRunner
  // outlives the invocation of the Page Speed engine, since the
  // engine inspects the live DOM during its execution.
  pagespeed::TestShellRunner runner;

  // The WebFrame object holds a reference to the page DOM, which we
  // use for the Page Speed DOM rules. Note that the WebFrame's
  // lifetime is scoped by the TestShellRunner instance, so it's
  // important that the TestShellRunner outlives the invocation of the
  // Pagespeed Engine (below).
  WebKit::WebFrame* frame = NULL;
  scoped_ptr<pagespeed::PagespeedInput> input(LoadPage(&runner, url, &frame));
  if (input == NULL || input->num_resources() == 0) {
    fprintf(stderr,
            "Unable to construct PagespeedInput for %s.\n", url);
    return false;
  }
  if (input->num_resources() == 1 &&
      input->GetResource(0).GetResponseStatusCode() != 200) {
    fprintf(stderr, "Non-200 response for %s.\n", url);
    return false;
  }

  input->SetPrimaryResourceUrl(gurl.spec());
  input->AcquireDomDocument(
      pagespeed::chromium::CreateDocument(frame->document()));
  input->AcquireImageAttributesFactory(
      new pagespeed::image_compression::ImageAttributesFactory());

  RunEngine(input.get());

  return true;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <url>\n\n", argv[0]);
    return 1;
  }

  std::string url(argv[1]);

  TestShellPlatformDelegate::PreflightArgs(&argc, &argv);
  CommandLine::Init(argc, argv);
  const CommandLine& parsed_command_line = *CommandLine::ForCurrentProcess();

  TestShellPlatformDelegate platform(parsed_command_line);

  // Only display WARNING and above on the console.
  logging::SetMinLogLevel(logging::LOG_WARNING);

  pagespeed::TestShellRunner::SetUp();
  bool result = RunPagespeed(url.c_str());
  pagespeed::TestShellRunner::TearDown();

  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
