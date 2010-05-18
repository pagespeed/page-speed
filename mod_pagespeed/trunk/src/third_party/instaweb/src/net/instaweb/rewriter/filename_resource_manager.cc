// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)
//     and sligocki@google.com (Shawn Ligocki)

#include "net/instaweb/rewriter/public/filename_resource_manager.h"

#include "net/instaweb/rewriter/public/file_input_resource.h"
#include "net/instaweb/rewriter/public/input_resource.h"
#include "net/instaweb/rewriter/public/output_resource.h"
#include "net/instaweb/util/public/content_type.h"
#include "net/instaweb/util/public/file_system.h"
#include "net/instaweb/util/public/google_url.h"
#include "net/instaweb/util/public/message_handler.h"
#include "net/instaweb/util/public/meta_data.h"
#include <string>
#include "net/instaweb/util/public/string_util.h"
#include "net/instaweb/util/public/url_fetcher.h"
#include "net/instaweb/rewriter/public/url_input_resource.h"

namespace net_instaweb {

FilenameResourceManager::FilenameResourceManager(FileSystem* file_system,
                                                 UrlFetcher* url_fetcher)
    : file_system_(file_system), url_fetcher_(url_fetcher) {
}

FilenameResourceManager::~FilenameResourceManager() {
  for (size_t i = 0; i < input_resources_.size(); ++i) {
    delete input_resources_[i];
  }
  for (size_t i = 0; i < output_resources_.size(); ++i) {
    delete output_resources_[i];
  }
}

void FilenameResourceManager::set_base_url(const StringPiece& url) {
  // TODO(sligocki): Is there any way to init GURL w/o alloc a whole new string?
  base_url_.reset(new GURL(url.as_string()));
}


void FilenameResourceManager::SetDefaultHeaders(const ContentType& content_type,
                                                MetaData* header) {
  header->set_major_version(1);
  header->set_minor_version(1);
  header->set_status_code(HttpStatus::OK);
  header->set_reason_phrase("OK");
  header->Add("Content-Type", content_type.mime_type());
}

InputResource* FilenameResourceManager::CreateInputResource(
    const StringPiece& input_url, MessageHandler* handler) {
  if (base_url_ == NULL) {
    handler->Message(kError, "CreateInputResource called before base_url set.");
    return NULL;
  }
  // Get absolute url based on the (possibly relative) input_url.
  GURL url = base_url_->Resolve(input_url.as_string());

  InputResource* resource = NULL;
  if (url.SchemeIs("http")) {
    // TODO(sligocki): Figure out if these are actually local by
    // seing if the serving path matches url_prefix_, in which case
    // we can do a local file read.
    const StringPiece url_string(url.spec().data(), url.spec().size());
    resource = new UrlInputResource(input_url, url_string, url_fetcher_);
  // TODO(sligocki): Probably shouldn't support file:// scheme.
  } else if (url.SchemeIsFile()) {
    const std::string& filename = url.path();
    // NOTE: This is raw filesystem access, no filename-encoding, etc.
    resource = new FileInputResource(input_url, filename, file_system_);
  } else {
    handler->Message(kError, "Unsupported scheme '%s' for url '%s'",
                     url.scheme().c_str(), url.spec().c_str());
  }
  if (resource != NULL) {
    input_resources_.push_back(resource);
  }
  return resource;
}

}  // namespace net_instaweb
