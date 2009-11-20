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

#include "nsArrayUtils.h" // for do_QueryElementAt
#include "nsCOMPtr.h"
#include "nsIInputStream.h"
#include "nsStringAPI.h"

#include "base/basictypes.h"

#include "pagespeed_json_input.h"
#include "pagespeed/core/engine.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/formatters/json_formatter.h"
#include "pagespeed/rules/rule_provider.h"

namespace pagespeed {

NS_IMPL_ISUPPORTS1(PageSpeedRules, IPageSpeedRules)

PageSpeedRules::PageSpeedRules() {}

PageSpeedRules::~PageSpeedRules() {}

NS_IMETHODIMP
PageSpeedRules::ComputeAndFormatResults(const char *data,
                                        nsIArray *inputStreams,
                                        char **_retval NS_OUTPARAM) {
  std::vector<std::string> contents;
  if (inputStreams != NULL) {
    PRUint32 length;
    inputStreams->GetLength(&length);
    for (PRUint32 i = 0; i < length; ++i) {
      nsCOMPtr<nsIInputStream> ptr(do_QueryElementAt(inputStreams, i));
      std::string content;
      if (ptr != NULL) {
        nsIInputStream &inputStream = *ptr;
        PRUint32 bytes_read = 0;
        do {
          char buffer[1024];
          inputStream.Read(buffer, arraysize(buffer), &bytes_read);
          content.append(buffer, static_cast<size_t>(bytes_read));
        } while (bytes_read > 0);
      }
      contents.push_back(content);
    }
  }

  std::vector<Rule*> rules;
  rule_provider::AppendCoreRules(true, &rules);

  Engine engine(rules);  // Ownership of rules is transferred to engine.
  engine.Init();

  PagespeedInput input;
  if (PopulateInputFromJSON(&input, data, contents)) {
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
