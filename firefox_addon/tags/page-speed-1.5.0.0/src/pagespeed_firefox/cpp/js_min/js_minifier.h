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

// JsMin interface implementation.

#ifndef JS_MINIFIER_H_
#define JS_MINIFIER_H_

#include "IJsMin.h"

#include "base/basictypes.h"

namespace pagespeed {

class JsMinifier : public IJsMin
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IJSMIN

  JsMinifier();

private:
  ~JsMinifier();

  DISALLOW_COPY_AND_ASSIGN(JsMinifier);
};

}  // namespace pagespeed

#endif  // JS_MINIFIER_H_
