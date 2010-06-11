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
#include "mod_pagespeed/pagespeed_process_context.h"
#include "mod_spdy/apache/log_message_handler.h"
#include "mod_spdy/apache/pool_util.h"
#include "pagespeed/cssmin/cssmin.h"
#include "pagespeed/image_compression/gif_reader.h"
#include "pagespeed/image_compression/jpeg_optimizer.h"
#include "pagespeed/image_compression/png_optimizer.h"
// The httpd header must be after the pagepseed_process_context.h. Otherwise,
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
                "Rewrite %s(%s%s) original=%d, minified=%zd",
                request->content_type,
                request->hostname, request->unparsed_uri,
                len, context->output.size());
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
                    "Minify %s(%s%s) original=%zd, minified=%zd",
                    request->content_type,
                    request->hostname, request->unparsed_uri,
                    context->input.size(), context->output.size());
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
                  "%5.2lf%% saved Minify %s(%s%s) original=%zd, minified=%zd",
                  saved_percent,  request->content_type,
                  request->hostname, request->unparsed_uri,
                  context->input.size(), context->output.size());
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
  const html_rewriter::PageSpeedProcessContext* context =
      html_rewriter::GetPageSpeedProcessContext(server);
  delete context;
  return APR_SUCCESS;
}


void pagespeed_child_init(apr_pool_t* pool, server_rec* server) {
  // Create fetcher.
  html_rewriter::CreatePageSpeedProcessContext(server);
  apr_pool_cleanup_register(pool, server, pagespeed_child_exit,
                            pagespeed_child_exit);
}

// This function is a callback and it declares what
// other functions should be called for request
// processing and configuration requests. This
// callback function declares the Handlers for
// other events.
void mod_pagespeed_register_hooks(apr_pool_t *p) {
  // Enable logging using pagespeed style
  mod_spdy::InstallLogMessageHandler();

  ap_register_output_filter(pagespeed_filter_name,
                            pagespeed_out_filter,
                            NULL,
                            AP_FTYPE_RESOURCE);

  ap_hook_child_init(pagespeed_child_init, NULL, NULL, APR_HOOK_LAST);
}

}  // namespace

extern "C" {
// Export our module so Apache is able to load us.
// See http://gcc.gnu.org/wiki/Visibility for more information.
#if defined(__linux)
#pragma GCC visibility push(default)
#endif

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
  NULL,
  NULL,
  NULL,
  mod_pagespeed_register_hooks,      // callback for registering hooks
};

#if defined(__linux)
#pragma GCC visibility pop
#endif
}  // extern "C"
