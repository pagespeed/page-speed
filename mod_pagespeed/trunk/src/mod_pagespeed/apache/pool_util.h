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

#ifndef MOD_PAGESPEED_APACHE_POOL_UTIL_H_
#define MOD_PAGESPEED_APACHE_POOL_UTIL_H_

#include "base/logging.h"
#include "third_party/apache_httpd/include/apr_pools.h"

namespace mod_pagespeed {

/**
 * Wrapper object that creates a new apr_pool_t and then destroys it when
 * deleted (handy for creating a local apr_pool_t on the stack).
 *
 * Example usage:
 *
 *   apr_status_t SomeFunction() {
 *     LocalPool local;
 *     if (local.status() != APR_SUCCESS) {
 *       return local.status();
 *     }
 *     char* buffer = apr_palloc(local.pool(), 1024);
 *     // Do stuff with buffer; it will dealloc when we leave this scope.
 *     return APR_SUCCESS;
 *   }
 */
class LocalPool {
 public:
  LocalPool() : pool_(NULL), status_(apr_pool_create(&pool_, NULL)) {
    DCHECK((pool_ == NULL) ^ (status_ == APR_SUCCESS));
  }

  ~LocalPool() {
    if (pool_) {
      DCHECK(status_ == APR_SUCCESS);
      apr_pool_destroy(pool_);
    }
  }

  apr_pool_t* pool() { return pool_; }
  apr_status_t status() const { return status_; }

 private:
  apr_pool_t* pool_;
  const apr_status_t status_;

  DISALLOW_COPY_AND_ASSIGN(LocalPool);
};

// Helper function for PoolRegisterDelete.
template <class T>
apr_status_t DeletionFunction(void* object) {
  delete static_cast<T*>(object);
  return APR_SUCCESS;
}

// Register a C++ object to be deleted with a pool.
template <class T>
void PoolRegisterDelete(apr_pool_t* pool, T* object) {
  // Note that the "child cleanup" argument below doesn't apply to us, so we
  // use apr_pool_cleanup_null, which is a no-op cleanup function.
  apr_pool_cleanup_register(pool, object,
                            DeletionFunction<T>,  // cleanup function
                            apr_pool_cleanup_null);  // child cleanup
}

}  // namespace mod_pagespeed

#endif  // MOD_PAGESPEED_APACHE_POOL_UTIL_H_
