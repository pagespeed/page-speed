// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "net/instaweb/rewriter/public/rewrite_driver.h"

#include <assert.h>
#include "net/instaweb/htmlparse/public/html_parse.h"
#include "net/instaweb/htmlparse/public/html_writer_filter.h"
#include "net/instaweb/rewriter/public/add_head_filter.h"
#include "net/instaweb/rewriter/public/base_tag_filter.h"
#include "net/instaweb/rewriter/public/cache_extender.h"
#include "net/instaweb/rewriter/public/css_combine_filter.h"
#include "net/instaweb/rewriter/public/filename_resource_manager.h"
#include "net/instaweb/rewriter/public/hash_resource_manager.h"
#include "net/instaweb/rewriter/public/img_rewrite_filter.h"
#include "net/instaweb/rewriter/public/outline_filter.h"
#include "net/instaweb/rewriter/public/resource_server.h"
#include "net/instaweb/util/public/url_async_fetcher.h"

namespace {

const char kCssCombiner[] = "cc";
const char kCacheExtender[] = "ce";
const char kFileSystem[] = "fs";
const char kImageCompression[] = "ic";

}  // namespace

namespace net_instaweb {

// TODO(jmarantz): Simplify the interface so we can just use
// asynchronous fetchers, employing FakeUrlAsyncFetcher as needed
// for running functional regression-tests where we don't mind blocking
// behavior.
RewriteDriver::RewriteDriver(HtmlParse* html_parse, FileSystem* file_system,
                             UrlFetcher* url_fetcher,
                             UrlAsyncFetcher* url_async_fetcher)
    : html_parse_(html_parse),
      file_system_(file_system),
      url_fetcher_(url_fetcher),
      url_async_fetcher_(url_async_fetcher),
      hasher_(NULL),
      html_writer_filter_(NULL) {
}

RewriteDriver::~RewriteDriver() {
}

void RewriteDriver::AddHead() {
  if (add_head_filter_ == NULL) {
    assert(html_writer_filter_ == NULL);
    add_head_filter_.reset(new AddHeadFilter(html_parse_));
    html_parse_->AddFilter(add_head_filter_.get());
  }
}

void RewriteDriver::AddBaseTagFilter() {
  AddHead();
  if (base_tag_filter_ == NULL) {
    assert(html_writer_filter_ == NULL);
    base_tag_filter_.reset(new BaseTagFilter(html_parse_));
    html_parse_->AddFilter(base_tag_filter_.get());
  }
}

void RewriteDriver::SetBaseUrl(const char* base) {
  if (base_tag_filter_ != NULL) {
    base_tag_filter_->set_base_url(base);
  }
  if (resource_manager_.get() != NULL) {
    resource_manager_->set_base_url(base);
  }
}

void RewriteDriver::SetFilenameResources(const std::string& file_prefix,
                                         const std::string& url_prefix,
                                         const int num_shards,
                                         const bool write_headers,
                                         const bool garble_filenames) {
  resource_manager_.reset(new FilenameResourceManager(
      file_prefix, url_prefix, num_shards, write_headers, garble_filenames,
      file_system_, url_fetcher_));
}

void RewriteDriver::SetHashResources(const std::string& file_prefix,
                                     const std::string& url_prefix,
                                     const int num_shards,
                                     const bool write_headers,
                                     const bool garble_filenames,
                                     Hasher* hasher) {
  hasher_ = hasher;
  resource_manager_.reset(new HashResourceManager(
      file_prefix, url_prefix, num_shards, write_headers, garble_filenames,
      file_system_, url_fetcher_, hasher_));
  resource_server_.reset(new ResourceServer(hasher,
                                            html_parse()->message_handler()));
}

void RewriteDriver::ExtendCacheLifetime() {
  assert(html_writer_filter_ == NULL);
  assert(resource_manager_.get() != NULL);
  assert(hasher_ != NULL);
  assert(cache_extender_ == NULL);
  cache_extender_.reset(
      new CacheExtender(kCacheExtender, html_parse_, resource_manager_.get(),
                        hasher_, resource_server_.get()));
  resource_filter_map_[kCacheExtender] = cache_extender_.get();
  html_parse_->AddFilter(cache_extender_.get());
}

void RewriteDriver::CombineCssFiles() {
  assert(html_writer_filter_ == NULL);
  assert(resource_manager_.get() != NULL);
  assert(css_combine_filter_.get() == NULL);
  css_combine_filter_.reset(
      new CssCombineFilter(kCssCombiner, html_parse_, resource_manager_.get()));
  resource_filter_map_[kCssCombiner] = css_combine_filter_.get();
  html_parse_->AddFilter(css_combine_filter_.get());
}

void RewriteDriver::OutlineResources(bool outline_styles,
                                     bool outline_scripts) {
  // TODO(sligocki): Use FatalError rather than assert.
  assert(html_writer_filter_ == NULL);
  assert(resource_manager_.get() != NULL);
  outline_filter_.reset(new OutlineFilter(html_parse_, resource_manager_.get(),
                                          outline_styles, outline_scripts));
  html_parse_->AddFilter(outline_filter_.get());
}

void RewriteDriver::RewriteImages() {
  assert(html_writer_filter_ == NULL);
  assert(resource_manager_.get() != NULL);
  assert(img_rewrite_filter_ == NULL);
  img_rewrite_filter_.reset(
      new ImgRewriteFilter(kImageCompression, html_parse_,
                           resource_manager_.get(), true));
  resource_filter_map_[kImageCompression] = img_rewrite_filter_.get();
  html_parse_->AddFilter(img_rewrite_filter_.get());
}

void RewriteDriver::SetWriter(Writer* writer) {
  if (html_writer_filter_ == NULL) {
    html_writer_filter_.reset(new HtmlWriterFilter(html_parse_));
  }
  html_parse_->AddFilter(html_writer_filter_.get());
  html_writer_filter_->set_writer(writer);
}

bool RewriteDriver::FetchResource(
    const char* resource,
    const MetaData& request_headers,
    MetaData* response_headers,
    Writer* writer,
    MessageHandler* message_handler,
    UrlAsyncFetcher::Callback* callback) {
  bool ret = false;
  const char* next_slash = strchr(resource, '/');
  if (next_slash != NULL) {
    StringPiece filter_id(resource, next_slash - resource);
    ResourceFilterMap::iterator p = resource_filter_map_.find(
        filter_id.as_string());
    if (p != resource_filter_map_.end()) {
      RewriteFilter* filter = p->second;
      ret = filter->Fetch(next_slash + 1, writer,
                          request_headers, response_headers,
                          url_async_fetcher_, message_handler, callback);
    }
  }
  return ret;
}

}  // namespace net_instaweb
