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

// Author: Bryan McQuade

#include "jsd_wrapper.h"

#include "base/scoped_ptr.h"

#include "nsCOMPtr.h"
#include "nsISupports.h"
#include "nsServiceManagerUtils.h"  // for do_GetService

namespace {
const char* kJsdContractStr = "@mozilla.org/js/jsd/debugger-service;1";
}

namespace activity {

JsdWrapper* JsdWrapper::Create() {
  nsresult rv = NS_OK;
  nsCOMPtr<nsISupports> jsd(do_GetService(kJsdContractStr, &rv));
  if (NS_FAILED(rv)) {
    return NULL;
  }

  // First try to create the version for FF3.5.
  scoped_ptr<JsdWrapper> wrapper(JsdWrapper::Create_3_5(jsd));
  if (wrapper != NULL) {
    return wrapper.release();
  }

  // First try to create the version for FF3.0.11.
  wrapper.reset(JsdWrapper::Create_3_0(jsd));
  if (wrapper != NULL) {
    return wrapper.release();
  }

  // If neither of the above matched, bail.
  return NULL;
}

}  // namespace activity
