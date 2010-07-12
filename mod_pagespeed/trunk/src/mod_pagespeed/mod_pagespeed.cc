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

#include <string>

#include "base/string_util.h"
#include "html_rewriter/html_rewriter.h"
#include "html_rewriter/html_rewriter_config.h"
#include "mod_pagespeed/instaweb_handler.h"
#include "mod_pagespeed/pagespeed_server_context.h"
#include "mod_spdy/apache/log_message_handler.h"
#include "mod_spdy/apache/pool_util.h"
#include "pagespeed/cssmin/cssmin.h"
#include "pagespeed/image_compression/gif_reader.h"
#include "pagespeed/image_compression/jpeg_optimizer.h"
#include "pagespeed/image_compression/png_optimizer.h"
#include "third_party/apache/apr/src/include/apr_strings.h"
// The httpd header must be after the pagepseed_server_context.h. Otherwise,
// the compiler will complain
// "strtoul_is_not_a_portable_function_use_strtol_instead".
#include "third_party/apache/httpd/src/include/httpd.h"
#include "third_party/apache/httpd/src/include/http_config.h"
#include "third_party/apache/httpd/src/include/http_log.h"
#include "third_party/apache/httpd/src/include/http_protocol.h"
#include "third_party/jsmin/cpp/jsmin.h"

using pagespeed::cssmin::MinifyCss;
using pagespeed::image_compression::GifReader;
using pagespeed::image_compression::OptimizeJpeg;
using pagespeed::image_compression::PngReader;
using pagespeed::image_compression::PngOptimizer;
using jsmin::MinifyJs;
using html_rewriter::HtmlRewriter;

extern "C" {
extern module AP_MODULE_DECLARE_DATA pagespeed_module;

const char* kPagespeedRewriteUrlPrefix = "PagespeedRewriteUrlPrefix";
const char* kPagespeedFetchProxy = "PagespeedFetchProxy";
const char* kPagespeedGeneratedFilePrefix = "PagespeedGeneratedFilePrefix";
const char* kPagespeedFileCachePath = "PagespeedFileCachePath";
const char* kPagespeedFetcherTimeoutMs = "PagespeedFetcherTimeOutMs";
const char* kPagespeedResourceTimeoutMs= "PagespeedResourceTimeOutMs";
}  // extern "C"


namespace {

const char* pagespeed_filter_name = "PAGESPEED";

enum ResourceType {UNKNOWN, HTML, JAVASCRIPT, CSS, GIF, PNG, JPEG};

// We use the following structure to keep the pagespeed module context. We
// accumulate buckets into the input string. When we receive the EOS bucket, we
// optimize the content to the string output, and re-bucket it.
struct PagespeedContext {
  std::string input;   // original content
  std::string output;  // content after pagespeed optimization
  HtmlRewriter* rewriter;
  apr_bucket_brigade* bucket_brigade;
};

typedef struct pagespeed_filter_config_t {
  html_rewriter::PageSpeedServerContext* server_context;
  const char* rewrite_url_prefix;
  const char* fetch_proxy;
  const char* generated_file_prefix;
  const char* file_cache_path;
  int64 fetcher_timeout_ms;
  int64 resource_timeout_ms;
} pagespeed_filter_config;

// Determine the resource type from a Content-Type string
ResourceType get_resource_type(const char* content_type) {
  ResourceType type = UNKNOWN;
  if (StartsWithASCII(content_type, "text/html", false)) {
    type = HTML;
  } else if (StartsWithASCII(content_type, "text/javascript", false) ||
             StartsWithASCII(content_type, "application/x-javascript",
                             false) ||
             StartsWithASCII(content_type, "application/javascript",
                             false)) {
    type = JAVASCRIPT;
  } else if (StartsWithASCII(content_type, "text/css", false)) {
    type = CSS;
  } else if (StartsWithASCII(content_type, "image/gif", false)) {
    type = GIF;
  } else if (StartsWithASCII(content_type, "image/png", false)) {
    type = PNG;
  } else if (StartsWithASCII(content_type, "image/jpg", false) ||
             StartsWithASCII(content_type, "image/jpeg", false)) {
    type = JPEG;
  } else {
    type = UNKNOWN;
  }
  return type;
}

// Check if pagespeed optimization rules applicable.
bool check_pagespeed_applicable(ap_filter_t* filter,
                                apr_bucket_brigade* bb,
                                ResourceType* type) {
  request_rec* request = filter->r;
  // We can't operate on Content-Ranges.
  if (apr_table_get(request->headers_out, "Content-Range") != NULL) {
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, APR_SUCCESS, request,
                  "Content-Range is not available");
    return false;
  }

  // Only support text/html, javascript, css, gif, png and jpeg.
  *type = get_resource_type(request->content_type);
  if (*type == UNKNOWN) {
    ap_log_rerror(APLOG_MARK, APLOG_INFO, APR_SUCCESS, request,
                  "Content-Type=%s URI=%s%s",
                  request->content_type, request->hostname,
                  request->unparsed_uri);
    return false;
  }

  return true;
}

// Optimize the resource.
bool perform_resource_optimization(const ResourceType resource_type,
                  const std::string& input, std::string* output) {
  bool success = false;
  switch (resource_type) {
    case JAVASCRIPT:
      success = MinifyJs(input, output);
      break;
    case CSS:
      success = MinifyCss(input, output);
      break;
    case GIF:
    {
      GifReader reader;
      success = PngOptimizer::OptimizePng(reader, input, output);
      break;
    }
    case PNG:
    {
      PngReader reader;
      success = PngOptimizer::OptimizePng(reader, input, output);
      break;
    }
    case JPEG:
      success = OptimizeJpeg(input, output);
      break;
    default:
      // should never be here.
      DCHECK(false);
      break;
  }
  return success;
}

// Create a new bucket from buf using HtmlRewriter.
// TODO(lsong): the content is copied multiple times. The buf is
// copied/processed to string output, then output is copied to new bucket.
apr_bucket* rewrite_html(ap_filter_t *filter, bool flush, const char* buf,
                         int len) {
  request_rec* request = filter->r;
  PagespeedContext* context = static_cast<PagespeedContext*>(filter->ctx);
  if (context == NULL) {
    LOG(DFATAL) << "Context is null";
    return NULL;
  }
  if (flush) {
    context->rewriter->Flush();
  } else {
    if (buf != NULL) {
      context->rewriter->Rewrite(buf, len);
      // Rewrite only without flush, no output.
      return NULL;
    } else {
      // when flush is false and buf is null it means we're at end of stream, so
      // we need to call finish
      context->rewriter->Finish();
    }
  }
  if (context->output.size() == 0) {
    return NULL;
  }
  ap_log_rerror(APLOG_MARK, APLOG_DEBUG, APR_SUCCESS, request,
                "Rewrite %s(%s%s) original=%d, minified=%d",
                request->content_type,
                request->hostname, request->unparsed_uri,
                len, static_cast<int>(context->output.size()));
  // Use the rewritten content. Create in heap since output will
  // be emptied for reuse.
  apr_bucket* bucket = apr_bucket_heap_create(
      context->output.data(),
      context->output.size(),
      NULL,
      request->connection->bucket_alloc);
  context->output.clear();
  return bucket;
}

// Create a new bucket from context->input using pagepseed optimizer.
apr_bucket* create_pagespeed_bucket(ap_filter_t *filter,
                                    ResourceType resource_type) {
  request_rec* request = filter->r;
  PagespeedContext* context = static_cast<PagespeedContext*>(filter->ctx);
  if (context == NULL) {
    LOG(DFATAL) << "Context is null";
    return NULL;
  }
  // Do pagespeed optimization on non-HTML resources.
  bool success = perform_resource_optimization(resource_type,
                                               context->input,
                                               &context->output);
  // Re-create the bucket.
  if (!success || context->input.size() <= context->output.size()) {
    if (!success) {
      ap_log_rerror(APLOG_MARK, APLOG_ERR, APR_SUCCESS, request,
                    "Minify %s failed. URI=%s%s",
                    request->content_type, request->hostname,
                    request->unparsed_uri);
    } else {
      ap_log_rerror(APLOG_MARK, APLOG_DEBUG, APR_SUCCESS, request,
                    "Minify %s(%s%s) original=%d, minified=%d",
                    request->content_type,
                    request->hostname, request->unparsed_uri,
                    static_cast<int>(context->input.size()),
                    static_cast<int>(context->output.size()));
    }
    // Use the original content. Here we use apr_bucket_transient_create to save
    // one copy of the content because context->input is persisted until the
    // request is being processed.
    return apr_bucket_transient_create(context->input.data(),
                                       context->input.size(),
                                       request->connection->bucket_alloc);
  } else {
    double saved_percent =
        100 - 100.0 * context->output.size() / context->input.size();
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, APR_SUCCESS, request,
                  "%5.2lf%% saved Minify %s(%s%s) original=%d, minified=%d",
                  saved_percent,  request->content_type,
                  request->hostname, request->unparsed_uri,
                  static_cast<int>(context->input.size()),
                  static_cast<int>(context->output.size()));
    if (resource_type == GIF) {
      // We minified the gif to png.
      ap_set_content_type(request, "image/png");
    }
    // Use the optimized content. Here we use apr_bucket_transient_create to
    // save one copy of the content because context->output is persisted until
    // the request is being processed.
    return apr_bucket_transient_create(context->output.data(),
                                       context->output.size(),
                                       request->connection->bucket_alloc);
  }
}

apr_status_t pagespeed_out_filter(ap_filter_t *filter, apr_bucket_brigade *bb) {
  // Do nothing if there is nothing, and stop passing to other filters.
  if (APR_BRIGADE_EMPTY(bb)) {
    return APR_SUCCESS;
  }

  // Check if pagespeed optimization applicable and get the resource type.
  ResourceType resource_type;
  if (!check_pagespeed_applicable(filter, bb, &resource_type)) {
    ap_remove_output_filter(filter);
    return ap_pass_brigade(filter->next, bb);
  }

  request_rec* request = filter->r;
  PagespeedContext* context = static_cast<PagespeedContext*>(filter->ctx);

  // Initialize pagespeed context structure.
  if (context == NULL) {
    filter->ctx = context = new PagespeedContext;
    mod_spdy::PoolRegisterDelete(request->pool, context);
    context->bucket_brigade = apr_brigade_create(
        request->pool,
        request->connection->bucket_alloc);
    if (resource_type == HTML) {
      context->rewriter = new HtmlRewriter(request,
                                           request->unparsed_uri,
                                           &context->output);
      mod_spdy::PoolRegisterDelete(request->pool, context->rewriter);
    }
    apr_table_unset(request->headers_out, "Content-Length");
    apr_table_unset(request->headers_out, "Content-MD5");
  }

  apr_bucket* new_bucket = NULL;
  while (!APR_BRIGADE_EMPTY(bb)) {
    apr_bucket* bucket = APR_BRIGADE_FIRST(bb);
    if (!APR_BUCKET_IS_METADATA(bucket)) {
      const char* buf = NULL;
      size_t bytes = 0;
      apr_status_t ret_code =
          apr_bucket_read(bucket, &buf, &bytes, APR_BLOCK_READ);
      if (ret_code == APR_SUCCESS) {
        if (resource_type == HTML) {
          new_bucket = rewrite_html(filter, false, buf, bytes);
          if (new_bucket != NULL) {
            APR_BRIGADE_INSERT_TAIL(context->bucket_brigade, new_bucket);
          }
        } else {
          // save the content of the bucket
          context->input.append(buf, bytes);
        }
      } else {
        // Read error, log the eror and return.
        ap_log_rerror(APLOG_MARK, APLOG_ERR, ret_code, request,
                      "Reading bucket failed (rcode=%d)", ret_code);
        return ret_code;
      }
      // Processed the bucket, now delete it.
      apr_bucket_delete(bucket);

    } else if (APR_BUCKET_IS_EOS(bucket)) {
      if (resource_type == HTML) {
        new_bucket = rewrite_html(filter, false, NULL, 0);
      } else if (context->input.empty()) {
        break;  // Nothing to be optimized.
      } else {
        new_bucket = create_pagespeed_bucket(filter, resource_type);
      }
      if (new_bucket != NULL) {
        APR_BRIGADE_INSERT_TAIL(context->bucket_brigade, new_bucket);
      }
      // Remove EOS from the old brigade, and insert into the new.
      APR_BUCKET_REMOVE(bucket);
      APR_BRIGADE_INSERT_TAIL(context->bucket_brigade, bucket);
      // OK, we have seen the EOS. Time to pass it along down the chain.
      return ap_pass_brigade(filter->next, context->bucket_brigade);

    } else if (APR_BUCKET_IS_FLUSH(bucket)) {
      if (resource_type == HTML) {
        new_bucket = rewrite_html(filter, true, NULL, 0);
        if (new_bucket != NULL) {
          APR_BRIGADE_INSERT_TAIL(context->bucket_brigade, new_bucket);
        }
        // Remove FLUSH from the old brigade, and insert into the new.
        APR_BUCKET_REMOVE(bucket);
        APR_BRIGADE_INSERT_TAIL(context->bucket_brigade, bucket);
        // OK, Time to flush, pass it along down the chain.
        return ap_pass_brigade(filter->next, context->bucket_brigade);
      } else {
        // Ignore the FLUSH bucket.
        apr_bucket_delete(bucket);
      }

    } else {
      // TODO(lsong): remove this log.
      ap_log_rerror(APLOG_MARK, APLOG_INFO, APR_SUCCESS, request,
                    "Unknown meta data");
      // Remove meta from the old brigade, and insert into the new.
      APR_BUCKET_REMOVE(bucket);
      APR_BRIGADE_INSERT_TAIL(context->bucket_brigade, bucket);
    }
  }

  apr_brigade_cleanup(bb);
  return APR_SUCCESS;
}

apr_status_t pagespeed_child_exit(void* data) {
  server_rec* server = static_cast<server_rec*>(data);
  const html_rewriter::PageSpeedServerContext* context =
      html_rewriter::GetPageSpeedServerContext(server);
  delete context;
  return APR_SUCCESS;
}


void pagespeed_child_init(apr_pool_t* pool, server_rec* server) {
  // Create PageSpeed context used by instaweb rewrite-driver.  This is
  // per-process, so we initialize all the server's context by iterating the
  // server lists in server->next.
  server_rec* next_server = server;
  while (next_server) {
    if (html_rewriter::CreatePageSpeedServerContext(next_server)) {
      apr_pool_cleanup_register(pool, next_server, pagespeed_child_exit,
                                pagespeed_child_exit);
    }
    next_server = next_server->next;
  }
}

// This function is a callback and it declares what
// other functions should be called for request
// processing and configuration requests. This
// callback function declares the Handlers for
// other events.
void mod_pagespeed_register_hooks(apr_pool_t *p) {
  // Enable logging using pagespeed style
  mod_spdy::InstallLogMessageHandler();

  // Use instaweb to handle generated resources.
  ap_hook_handler(mod_pagespeed::instaweb_handler, NULL, NULL, APR_HOOK_MIDDLE);
  ap_register_output_filter(pagespeed_filter_name,
                            pagespeed_out_filter,
                            NULL,
                            AP_FTYPE_RESOURCE);

  ap_hook_child_init(pagespeed_child_init, NULL, NULL, APR_HOOK_LAST);
}

pagespeed_filter_config* get_pagespeed_config(server_rec* server) {
  return static_cast<pagespeed_filter_config*> ap_get_module_config(
      server->module_config, &pagespeed_module);
}

void* mod_pagespeed_create_server_config(
    apr_pool_t* pool,
    server_rec* server) {
  pagespeed_filter_config* config = static_cast<pagespeed_filter_config*> (
      apr_pcalloc(pool, sizeof(pagespeed_filter_config)));
  config->server_context = NULL;
  config->rewrite_url_prefix = NULL;
  config->fetch_proxy = NULL;
  config->generated_file_prefix = NULL;
  config->file_cache_path = NULL;
  config->fetcher_timeout_ms = -1;
  config->resource_timeout_ms = -1;
  return config;
}

}  // namespace

// Getters for mod_pagespeed configuration.
namespace html_rewriter {

PageSpeedServerContext* mod_pagespeed_get_config_server_context(
    server_rec* server) {
  pagespeed_filter_config* config = get_pagespeed_config(server);
  return config->server_context;
}
void mod_pagespeed_set_config_server_context(
    server_rec* server, PageSpeedServerContext* context) {
  pagespeed_filter_config* config = get_pagespeed_config(server);
  config->server_context = context;
}


const char* mod_pagespeed_get_config_str(server_rec* server,
                                         const char* directive) {
  pagespeed_filter_config* config = get_pagespeed_config(server);
  if (strcasecmp(directive, kPagespeedRewriteUrlPrefix) == 0) {
    return config->rewrite_url_prefix;
  } else if (strcasecmp(directive, kPagespeedFetchProxy) == 0) {
    return config->fetch_proxy;
  } else if (strcasecmp(directive, kPagespeedGeneratedFilePrefix) == 0) {
    return config->generated_file_prefix;
  } else if (strcasecmp(directive, kPagespeedFileCachePath) == 0) {
    return config->file_cache_path;
  } else {
    return NULL;
  }
}

int64 mod_pagespeed_get_config_int(server_rec* server,
                                     const char* directive) {
  pagespeed_filter_config* config = get_pagespeed_config(server);
  if (strcasecmp(directive, kPagespeedFetcherTimeoutMs) == 0) {
    return config->fetcher_timeout_ms;
  } else if (strcasecmp(directive, kPagespeedResourceTimeoutMs) == 0) {
    return config->resource_timeout_ms;
  } else {
    return -1;
  }
}

}  // namespace html_rewriter

extern "C" {
// Export our module so Apache is able to load us.
// See http://gcc.gnu.org/wiki/Visibility for more information.
#if defined(__linux)
#pragma GCC visibility push(default)
#endif

static const char* mod_pagespeed_config_one_string(cmd_parms* cmd, void* data,
                                                   const char* arg) {
  pagespeed_filter_config* config = get_pagespeed_config(cmd->server);
  const char* directive = (cmd->directive->directive);
  if (strcasecmp(directive, kPagespeedRewriteUrlPrefix) == 0) {
    config->rewrite_url_prefix = apr_pstrdup(cmd->pool, arg);
  } else if (strcasecmp(directive, kPagespeedFetchProxy) == 0) {
    config->fetch_proxy = apr_pstrdup(cmd->pool, arg);
  } else if (strcasecmp(directive, kPagespeedGeneratedFilePrefix) == 0) {
    config->generated_file_prefix = apr_pstrdup(cmd->pool, arg);
  } else if (strcasecmp(directive, kPagespeedFileCachePath) == 0) {
    config->file_cache_path = apr_pstrdup(cmd->pool, arg);
  } else if (strcasecmp(directive, kPagespeedFetcherTimeoutMs) == 0) {
    config->fetcher_timeout_ms = static_cast<int64>(
        apr_strtoi64(arg, NULL, 10));
  } else if (strcasecmp(directive, kPagespeedResourceTimeoutMs) == 0) {
    config->resource_timeout_ms = static_cast<int64>(
        apr_strtoi64(arg, NULL, 10));
  } else {
    return "Unknown driective.";
  }
  return NULL;
}

static const command_rec mod_pagespeed_filter_cmds[] = {
  AP_INIT_TAKE1(kPagespeedRewriteUrlPrefix,
                reinterpret_cast<const char*(*)()>(
                    mod_pagespeed_config_one_string),
                NULL, RSRC_CONF,
                "Set the url prefix"),
  AP_INIT_TAKE1(kPagespeedFetchProxy,
                reinterpret_cast<const char*(*)()>(
                    mod_pagespeed_config_one_string),
                NULL, RSRC_CONF,
                "Set the fetch proxy"),
  AP_INIT_TAKE1(kPagespeedGeneratedFilePrefix,
                reinterpret_cast<const char*(*)()>(
                    mod_pagespeed_config_one_string),
                NULL, RSRC_CONF,
                "Set generated file's prefix"),
  AP_INIT_TAKE1(kPagespeedFileCachePath,
                reinterpret_cast<const char*(*)()>(
                    mod_pagespeed_config_one_string),
                NULL, RSRC_CONF,
                "Set the path for file cache"),
  AP_INIT_TAKE1(kPagespeedFetcherTimeoutMs,
                reinterpret_cast<const char*(*)()>(
                    mod_pagespeed_config_one_string),
                NULL, RSRC_CONF,
                "Set internal fetcher timeout in milisecons"),
  AP_INIT_TAKE1(kPagespeedResourceTimeoutMs,
                reinterpret_cast<const char*(*)()>(
                    mod_pagespeed_config_one_string),
                NULL, RSRC_CONF,
                "Set resource fetcher timeout in milisecons"),
  {NULL}
};
// Declare and populate the module's data structure.  The
// name of this structure ('pagespeed_module') is important - it
// must match the name of the module.  This structure is the
// only "glue" between the httpd core and the module.
module AP_MODULE_DECLARE_DATA pagespeed_module = {
  // Only one callback function is provided.  Real
  // modules will need to declare callback functions for
  // server/directory configuration, configuration merging
  // and other tasks.
  STANDARD20_MODULE_STUFF,
  NULL,
  NULL,
  mod_pagespeed_create_server_config,
  NULL,
  mod_pagespeed_filter_cmds,
  mod_pagespeed_register_hooks,      // callback for registering hooks
};

#if defined(__linux)
#pragma GCC visibility pop
#endif
}  // extern "C"
