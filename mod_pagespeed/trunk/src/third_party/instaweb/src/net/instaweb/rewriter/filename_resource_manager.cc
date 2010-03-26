// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)
//     and sligocki@google.com (Shawn Ligocki)

#include "net/instaweb/rewriter/public/filename_resource_manager.h"

#include <assert.h>
#include <stdio.h>  // for snprintf
#include <string.h>
#include <string>
#include "net/instaweb/htmlparse/public/file_system.h"
#include "net/instaweb/rewriter/public/filename_output_resource.h"
#include "net/instaweb/rewriter/public/file_input_resource.h"
#include "net/instaweb/rewriter/public/input_resource.h"
#include "net/instaweb/rewriter/public/output_resource.h"
#include "net/instaweb/rewriter/public/url_fetcher.h"
#include "net/instaweb/rewriter/public/url_input_resource.h"

namespace {
const std::string kHttpUrlPrefix = "http://";
const std::string kFileUrlPrefix = "file://";
const int kFileUrlPrefixSize = sizeof(kFileUrlPrefix);
}

namespace net_instaweb {

FilenameResourceManager::FilenameResourceManager(
    const std::string& file_prefix, const std::string& url_prefix,
    int num_shards, FileSystem* file_system, UrlFetcher* url_fetcher)
    : file_prefix_(file_prefix),
      url_prefix_(url_prefix),
      num_shards_(num_shards),
      resource_id_(0),
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

OutputResource* FilenameResourceManager::CreateOutputResource(
    const std::string& suffix) {
  int id = resource_id_++;
  char id_string[100];
  snprintf(id_string, sizeof(id_string), "%d", id);

  std::string url = url_prefix_ + id_string + suffix;
  std::string filename = file_prefix_ + id_string + suffix;

  FilenameOutputResource* resource1 =
      new FilenameOutputResource(url, filename, file_system_);
  OutputResource* resource = resource1;
  output_resources_.push_back(resource);
  return resource;
}

InputResource* FilenameResourceManager::CreateInputResource(
    const std::string& url) {
  InputResource* resource = NULL;

  // TODO(jmarantz): use googleurl for this.
  if (url.find(kHttpUrlPrefix) == 0) {
    // TODO(sligocki): figure out if these are actually local.
    resource = new UrlInputResource(url, url_fetcher_);
  } else if (url.find(kFileUrlPrefix) == 0) {
    std::string filename = url.substr(kFileUrlPrefixSize);
    resource = new FileInputResource(url, filename, file_system_);
  } else {
    // Also handle relative references, for the moment assuming
    // that we will be reading these resources from the file system.

    std::string filename(base_dir_);
    if (!filename.empty()) {
      filename += '/';
    }
    // TODO(sligocki): I don't think that this is correct if we are in a
    // subdirectory. We should use the googleurl anyway.
    filename += url;
    resource = new FileInputResource(url, filename, file_system_);
  }
  if (resource != NULL) {
    input_resources_.push_back(resource);
  }
  return resource;
}
}
