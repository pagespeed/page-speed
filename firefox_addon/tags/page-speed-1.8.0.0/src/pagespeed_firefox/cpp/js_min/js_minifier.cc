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

#include "js_minifier.h"

#include <string>

#include "nsStringAPI.h"

#include "third_party/jsmin/cpp/jsmin.h"

/* Implementation file */
NS_IMPL_ISUPPORTS1(pagespeed::JsMinifier, IJsMin)

namespace pagespeed {

JsMinifier::JsMinifier() {
}

JsMinifier::~JsMinifier() {
}

NS_IMETHODIMP JsMinifier::MinifyJs(const char *input, char **_retval) {
  std::string minified_js;
  if (!jsmin::MinifyJs(input, &minified_js)) {
    return NS_ERROR_FAILURE;
  }

  nsCString retval(minified_js.c_str(), minified_js.length());
  *_retval = NS_CStringCloneData(retval);
  return NS_OK;
}

}  // namespace pagespeed
