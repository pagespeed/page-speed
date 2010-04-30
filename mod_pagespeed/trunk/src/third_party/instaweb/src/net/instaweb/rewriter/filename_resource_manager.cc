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
#include "net/instaweb/util/public/filename_encoder.h"
#include "net/instaweb/util/public/google_url.h"
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
    const StringPiece& file_prefix, const StringPiece& url_prefix,
    const int num_shards, const bool write_headers, FileSystem* file_system,
    FilenameEncoder* filename_encoder, UrlFetcher* url_fetcher)
    : num_shards_(num_shards),
      resource_id_(0),
      write_http_headers_(write_headers),
      file_system_(file_system),
      filename_encoder_(filename_encoder),
      url_fetcher_(url_fetcher) {
  file_prefix.CopyToString(&file_prefix_);
  url_prefix.CopyToString(&url_prefix_);
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
  header->set_status_code(HttpStatus::OK);
  header->set_reason_phrase("OK");
  header->Add("Content-Type", content_type.mime_type());
}

OutputResource* FilenameResourceManager::NamedOutputResource(
    const StringPiece& name,
    const ContentType& content_type) {
  const char* extension = content_type.file_extension();

  std::string url = StrCat(url_prefix_, name, extension);

  std::string raw_ending = StrCat(name, extension);
  std::string filename;
  filename_encoder_->Encode(file_prefix_, raw_ending, &filename);

  OutputResource* resource =
      new FilenameOutputResource(url, filename, write_http_headers_,
                                 file_system_);
  SetDefaultHeaders(content_type, resource->metadata());

  output_resources_.push_back(resource);
  return resource;
}

OutputResource* FilenameResourceManager::GenerateOutputResource(
    const ContentType& content_type) {
  int id = resource_id_++;
  std::string id_string = IntegerToString(id);
  return NamedOutputResource(id_string, content_type);
}

InputResource* FilenameResourceManager::CreateInputResource(
    const StringPiece& input_url) {
  InputResource* resource = NULL;

  scoped_ptr<GURL> gurl(new GURL(input_url.as_string()));
  std::string url;
  input_url.CopyToString(&url);

  if (gurl->scheme().empty()) {
    // TODO(jmarantz): check behavior if input_url does not begin with a slash.
    url = StrCat(base_url_, input_url);
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
