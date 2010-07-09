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


#ifndef MOD_PAGESPEED_MOD_PAGESPEED_H_
#define MOD_PAGESPEED_MOD_PAGESPEED_H_

// Forward declaration.
struct server_rec;

#ifdef __cplusplus
extern "C" {
#endif
// Pagespeed directive names.
extern const char* kPagespeedRewriteUrlPrefix;
extern const char* kPagespeedFetchProxy;
extern const char* kPagespeedGeneratedFilePrefix;
extern const char* kPagespeedFileCachePath;
extern const char* kPagespeedFetcherTimeoutMs;
extern const char* kPagespeedResourceTimeoutMs;
#ifdef __cplusplus
}  // extern "C"
#endif



namespace html_rewriter {

class PageSpeedProcessContext;

PageSpeedProcessContext* mod_pagespeed_get_config_process_context(
    server_rec* server);
void mod_pagespeed_set_config_process_context(server_rec* server,
                                              PageSpeedProcessContext* context);

const char* mod_pagespeed_get_config_str(server_rec* server,
                                         const char* directive);
int64_t mod_pagespeed_get_config_int(server_rec* server,
                                     const char* directive);

}  // namespace html_rewriter

#endif  // MOD_PAGESPEED_MOD_PAGESPEED_H_
