// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)
//     and sligocki@google.com (Shawn Ligocki)

#include "net/instaweb/rewriter/public/filename_resource_manager.h"

#include <assert.h>
#include <stdio.h>  // for snprintf
#include <string.h>
#include "base/scoped_ptr.h"
#include "net/instaweb/rewriter/public/filename_output_resource.h"
#include "net/instaweb/rewriter/public/file_input_resource.h"
#include "net/instaweb/rewriter/public/input_resource.h"
#include "net/instaweb/rewriter/public/output_resource.h"
#include "net/instaweb/util/public/content_type.h"
#include "net/instaweb/util/public/file_system.h"
#include "net/instaweb/util/public/google_url.h"
#include "net/instaweb/util/public/http_dump_util.h"
#include <string>
#include "net/instaweb/util/public/string_util.h"
#include "net/instaweb/util/public/url_fetcher.h"
#include "net/instaweb/rewriter/public/url_input_resource.h"

namespace {
const std::string kHttpUrlPrefix = "http://";
const std::string kFileUrlPrefix = "file://";
const int kFileUrlPrefixSize = sizeof(kFileUrlPrefix);
}

namespace net_instaweb {

FilenameResourceManager::FilenameResourceManager(
    const std::string& file_prefix, const std::string& url_prefix,
    const int num_shards, const bool write_headers, const bool garble_filenames,
    FileSystem* file_system, UrlFetcher* url_fetcher)
    : file_prefix_(file_prefix),
      url_prefix_(url_prefix),
      num_shards_(num_shards),
      resource_id_(0),
      write_http_headers_(write_headers),
      garble_filenames_(garble_filenames),
      file_system_(file_system),
      url_fetcher_(url_fetcher) {
}

FilenameResourceManager::~FilenameResourceManager() {
  for (size_t i = 0; i < input_resources_.size(); ++i) {
    delete input_resources_[i];
  }
  for (size_t i = 0; i < output_resources_.size(); ++i) {
    delete output_resources_[i];
  }
}

void FilenameResourceManager::SetDefaultHeaders(const ContentType& content_type,
                                                MetaData* header) {
  header->set_major_version(1);
  header->set_minor_version(1);
  header->set_status_code(200);
  header->set_reason_phrase("OK");
  header->Add("Content-Type", content_type.mime_type());
}

OutputResource* FilenameResourceManager::CreateOutputResource(
    const ContentType& content_type) {
  int id = resource_id_++;
  char id_string[100];
  snprintf(id_string, sizeof(id_string), "%d", id);
  const char* extension = content_type.file_extension();

  std::string url = StrCat(url_prefix_, id_string, extension);
  std::string filename = file_prefix_;
  if (garble_filenames_) {
    std::string ungarbled_end = StrCat(id_string, extension);
    // Appends garbled end.
    latencylab::EscapeNonAlphanum(ungarbled_end, &filename);
  } else {
    filename += id_string;
    filename += extension;
  }

  OutputResource* resource =
      new FilenameOutputResource(url, filename, write_http_headers_,
                                 file_system_);
  SetDefaultHeaders(content_type, resource->metadata());

  output_resources_.push_back(resource);
  return resource;
}

InputResource* FilenameResourceManager::CreateInputResource(
    const std::string& input_url) {
  InputResource* resource = NULL;

  scoped_ptr<GURL> gurl(new GURL(input_url));
  std::string url = input_url;

  if (gurl->scheme().empty()) {
    // TODO(jmarantz): check behavior if input_url does not begin with a slash.
    url = base_url_ + input_url;
    gurl.reset(new GURL(url));
  }

  if (gurl->SchemeIs("http")) {
    // TODO(sligocki): figure out if these are actually local by
    // seing if the serving path matches url_prefix_, in which case
    // we can do a local file read.
    resource = new UrlInputResource(url, url_fetcher_);
  } else if (gurl->SchemeIsFile()) {
    const std::string& filename = gurl->path();
    resource = new FileInputResource(url, filename, file_system_);
  }
  if (resource != NULL) {
    input_resources_.push_back(resource);
  }
  return resource;
}

}  // namespace net_instaweb
