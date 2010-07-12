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

#ifndef MOD_PAGESPEED_PAGESPEED_SERVER_CONTEXT_H_
#define MOD_PAGESPEED_PAGESPEED_SERVER_CONTEXT_H_

#include "base/scoped_ptr.h"
#include "html_rewriter/apache_rewrite_driver_factory.h"

// Forward declaration.
struct server_rec;

// Forward declaration.
namespace net_instaweb {
class RewriteDriverFactory;
}

namespace html_rewriter {



class PageSpeedServerContext {
 public:
  PageSpeedServerContext();
  ~PageSpeedServerContext();
  void set_rewrite_driver_factory(
      net_instaweb::ApacheRewriteDriverFactory* factory) {
    rewrite_driver_factory_.reset(factory);
  }
  net_instaweb::ApacheRewriteDriverFactory* rewrite_driver_factory() {
    return rewrite_driver_factory_.get();
  }

 private:
  scoped_ptr<net_instaweb::ApacheRewriteDriverFactory> rewrite_driver_factory_;
};

PageSpeedServerContext* GetPageSpeedServerContext(server_rec* server);
bool CreatePageSpeedServerContext(server_rec* server);

}  // namespace html_rewriter

#endif  // MOD_PAGESPEED_PAGESPEED_SERVER_CONTEXT_H_
