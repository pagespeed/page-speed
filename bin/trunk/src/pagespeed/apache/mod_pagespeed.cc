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

#include <sstream>
#include <string>
#include <vector>

#include "base/logging.h"
#include "base/stl_util-inl.h"
#include "googleurl/src/gurl.h"
#include "pagespeed/pagespeed_input_populator.h"
#include "pagespeed/test_shell_runner.h"
#include "third_party/apache/apr/src/include/apr_strings.h"
#include "third_party/apache/httpd/src/include/httpd.h"
#include "third_party/apache/httpd/src/include/http_core.h"
#include "third_party/apache/httpd/src/include/http_config.h"
#include "third_party/apache/httpd/src/include/http_log.h"
#include "third_party/apache/httpd/src/include/http_protocol.h"
#include "third_party/apache/httpd/src/include/http_request.h"
#include "third_party/libpagespeed/src/pagespeed/core/engine.h"
#include "third_party/libpagespeed/src/pagespeed/core/pagespeed_input.h"
#include "third_party/libpagespeed/src/pagespeed/core/rule.h"
#include "third_party/libpagespeed/src/pagespeed/formatters/json_formatter.h"
#include "third_party/libpagespeed/src/pagespeed/image_compression/image_attributes_factory.h"
#include "third_party/libpagespeed/src/pagespeed/rules/rule_provider.h"

namespace {

// 30 seconds
const int kTimeoutMillis = 30 * 1000;

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

bool RunPagespeed(const char* url, std::ostream* out_json) {
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
  if (input->num_resources() == 1 &&
      input->GetResource(0).GetResponseStatusCode() != 200) {
    fprintf(stderr, "Non-200 response for %s.\n", url);
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
      new pagespeed::formatters::JsonFormatter(out_json, NULL));

  engine.ComputeAndFormatResults(*input.get(), formatter.get());
  return true;
}

int pagespeed_handler(request_rec* r) {
  // Check if the request is for our pagespeed content generator
  // Decline the request so that other handler may process
  if (!r->handler || strcmp(r->handler, "pagespeed")) {
    ap_log_rerror(APLOG_MARK, APLOG_WARNING, APR_SUCCESS, r,
                  "Not pagespeed request.");
    return DECLINED;
  }

  // Only handle GET request
  if (r->method_number != M_GET) {
    ap_log_rerror(APLOG_MARK, APLOG_WARNING, APR_SUCCESS, r,
                  "Not GET request: %d.", r->method_number);
    return HTTP_METHOD_NOT_ALLOWED;
  }

  const char* url = r->parsed_uri.query;
  if (url == NULL) {
    ap_log_rerror(APLOG_MARK, APLOG_WARNING, APR_SUCCESS, r,
                  "No query string");
    r->status = HTTP_INTERNAL_SERVER_ERROR;
    return OK;
  }

  std::ostringstream json;
  if (!RunPagespeed(url, &json)) {
    r->status = HTTP_INTERNAL_SERVER_ERROR;
    return OK;
  }
  const std::string json_str = json.str();

  r->status = HTTP_OK;
  char* content_type = apr_pstrdup(r->pool, "application/x-json");
  ap_set_content_type(r, content_type);
  ap_set_content_length(r, json_str.size());
  ap_rwrite(json_str.c_str(), json_str.size(), r);
  return OK;
}

// Run the TestShell's TearDown code upon process shutdown.
apr_status_t tear_down(void*) {
  pagespeed::TestShellRunner::TearDown();
  return APR_SUCCESS;
}

void pagespeed_hook(apr_pool_t* p) {
  // Invoke TestShell's SetUp code, and register a callback for
  // TestShell's TearDown code.
  pagespeed::TestShellRunner::SetUp();
  apr_pool_cleanup_register(p, NULL, tear_down, NULL);

  // Install our handler.
  ap_hook_handler(pagespeed_handler, NULL, NULL, APR_HOOK_MIDDLE);
}

}  // namespace

extern "C" {
  // Export our module so Apache is able to load us.
  // See http://gcc.gnu.org/wiki/Visibility for more information.
#if defined(__linux)
#pragma GCC visibility push(default)
#endif

  // Declare our module object (note that "module" is a typedef for "struct
  // module_struct"; see http_config.h for the definition of module_struct).
  module AP_MODULE_DECLARE_DATA pagespeed_module = {
    // This next macro indicates that this is a (non-MPM) Apache 2.0 module
    // (the macro actually expands to multiple comma-separated arguments; see
    // http_config.h for the definition):
    STANDARD20_MODULE_STUFF,

    // These next four arguments are callbacks, but we currently don't need
    // them, so they are left null:
    NULL,  // create per-directory config structure
    NULL,  // merge per-directory config structures
    NULL,  // create per-server config structure
    NULL,  // merge per-server config structures

    // This argument supplies a table describing the configuration directives
    // implemented by this module (however, we don't currently have any):
    NULL,

    // Finally, this function will be called to register hooks for this module:
    pagespeed_hook
  };

#if defined(__linux)
#pragma GCC visibility pop
#endif
}
