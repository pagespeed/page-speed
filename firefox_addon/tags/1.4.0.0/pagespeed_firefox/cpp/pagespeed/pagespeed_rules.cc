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

// Author: Matthew Steele

#include "pagespeed_rules.h"

#include <sstream>
#include <vector>

#include "nsStringAPI.h"

#include "pagespeed_json_input.h"
#include "pagespeed/core/engine.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/formatters/json_formatter.h"

namespace pagespeed {

NS_IMPL_ISUPPORTS1(PageSpeedRules, IPageSpeedRules)

PageSpeedRules::PageSpeedRules() {}

PageSpeedRules::~PageSpeedRules() {}

NS_IMETHODIMP
PageSpeedRules::ComputeAndFormatResults(const char *data,
                                        char **_retval NS_OUTPARAM) {
  std::vector<Rule*> rules;
  // TODO Add rules, or use rule_provider.

  Engine engine(rules);  // Ownership of rules is transferred to engine.
  engine.Init();

  PagespeedInput input;
  if (PopulateInputFromJSON(&input, data)) {
    std::stringstream stream;
    formatters::JsonFormatter formatter(&stream);
    engine.ComputeAndFormatResults(input, &formatter);

    nsCString retval(stream.str().c_str());
    *_retval = NS_CStringCloneData(retval);
    return NS_OK;
  } else {
    return NS_ERROR_FAILURE;
  }
}

}  // namespace pagespeed
