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

#include <string>

#include "nsStringAPI.h"

namespace pagespeed {

NS_IMPL_ISUPPORTS1(PageSpeedRules, IPageSpeedRules)

PageSpeedRules::PageSpeedRules() {}

PageSpeedRules::~PageSpeedRules() {}

NS_IMETHODIMP
PageSpeedRules::ComputeAndFormatResults(const char *data,
                                        char **_retval NS_OUTPARAM) {
  std::string result("\"Hello, world!\"");
  nsCString retval(result.c_str());
  *_retval = NS_CStringCloneData(retval);
  return NS_OK;
}

}  // namespace pagespeed
