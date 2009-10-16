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

// Basic implementation of JsdWrapper that delegates to an actual
// jsdIDebuggerService interface. This cc file is intended to be
// #included into another cc file, and not compiled directly.

#include "base/scoped_ptr.h"
#include "jsd_wrapper.h"

#include "nsCOMPtr.h"
#include "nsISupports.h"

namespace {

// Implementation of JsdWrapper that delegates to a
// jsdIDebuggerService implementation.
template<typename jsdTraits>
class JsdWrapperTmpl : public activity::JsdWrapper {
 public:
  // Create a JsdWrapper instance if jsdTraits is compatible with the active
  // jsdIDebuggerService, else NULL.
  static activity::JsdWrapper* Create(nsISupports *jsd);

  NS_IMETHOD SetScriptHook(nsISupports *script_hook_supports);
  NS_IMETHOD SetTopLevelHook(nsISupports *top_level_hook_supports);
  NS_IMETHOD SetFunctionHook(nsISupports *function_hook_supports);
  NS_IMETHOD GetFlags(PRUint32 *flags);
  NS_IMETHOD SetFlags(PRUint32 flags);

 private:
  explicit JsdWrapperTmpl(nsISupports *jsd);
  bool IsSupportedJsd() const;

  nsCOMPtr<typename jsdTraits::jsdIDebuggerService> jsd_;
};

template<typename jsdTraits>
activity::JsdWrapper* JsdWrapperTmpl<jsdTraits>::Create(nsISupports *jsd) {
  scoped_ptr<JsdWrapperTmpl> wrapper(new JsdWrapperTmpl<jsdTraits>(jsd));
  if (wrapper->IsSupportedJsd()) {
    return wrapper.release();
  }

  return NULL;
}

template<typename jsdTraits>
JsdWrapperTmpl<jsdTraits>::JsdWrapperTmpl(nsISupports *jsd) {
  nsresult rv = NS_OK;
  jsd_ = do_QueryInterface(jsd, &rv);
  if (NS_FAILED(rv)) {
    jsd_ = NULL;
  }
}

template<typename jsdTraits>
NS_IMETHODIMP JsdWrapperTmpl<jsdTraits>::SetScriptHook(
    nsISupports *script_hook_supports) {
  if (script_hook_supports == NULL) {
    return jsd_->SetScriptHook(NULL);
  }
  nsresult rv = NS_OK;
  nsCOMPtr<typename jsdTraits::jsdIScriptHook> script_hook(
      do_QueryInterface(script_hook_supports, &rv));
  if (NS_FAILED(rv)) {
    return rv;
  }
  return jsd_->SetScriptHook(script_hook);
}

template<typename jsdTraits>
NS_IMETHODIMP JsdWrapperTmpl<jsdTraits>::SetTopLevelHook(
    nsISupports *top_level_hook_supports) {
  if (top_level_hook_supports == NULL) {
    return jsd_->SetTopLevelHook(NULL);
  }
  nsresult rv = NS_OK;
  nsCOMPtr<typename jsdTraits::jsdICallHook> call_hook(
      do_QueryInterface(top_level_hook_supports, &rv));
  if (NS_FAILED(rv)) {
    return rv;
  }
  return jsd_->SetTopLevelHook(call_hook);
}

template<typename jsdTraits>
NS_IMETHODIMP JsdWrapperTmpl<jsdTraits>::SetFunctionHook(
    nsISupports *function_hook_supports) {
  if (function_hook_supports == NULL) {
    return jsd_->SetFunctionHook(NULL);
  }
  nsresult rv = NS_OK;
  nsCOMPtr<typename jsdTraits::jsdICallHook> call_hook(
      do_QueryInterface(function_hook_supports, &rv));
  if (NS_FAILED(rv)) {
    return rv;
  }
  return jsd_->SetFunctionHook(call_hook);
}

template<typename jsdTraits>
NS_IMETHODIMP JsdWrapperTmpl<jsdTraits>::GetFlags(PRUint32 *flags) {
  return jsd_->GetFlags(flags);
}

template<typename jsdTraits>
NS_IMETHODIMP JsdWrapperTmpl<jsdTraits>::SetFlags(PRUint32 flags) {
  return jsd_->SetFlags(flags);
}

template<typename jsdTraits>
bool JsdWrapperTmpl<jsdTraits>::IsSupportedJsd() const {
  return jsd_ != NULL;
}

}  // namespace
