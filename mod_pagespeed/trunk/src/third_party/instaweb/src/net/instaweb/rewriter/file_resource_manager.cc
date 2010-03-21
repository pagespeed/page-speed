// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "net/instaweb/rewriter/public/file_resource_manager.h"

#include <assert.h>
#include <stdio.h>  // for snprintf
#include <string.h>
#include <string>
#include "net/instaweb/htmlparse/public/file_system.h"
#include "net/instaweb/htmlparse/public/file_writer.h"
#include "net/instaweb/rewriter/public/outline_resource.h"
#include "net/instaweb/rewriter/public/file_resource.h"
#include "net/instaweb/rewriter/public/sprite_resource.h"

namespace {
const char kFileUrlPrefix[] = "file://";
const int kFileUrlPrefixSize = sizeof(kFileUrlPrefix);
}

namespace net_instaweb {
FileResourceManager::FileResourceManager(
    const char* file_prefix, const char* server_prefix, int num_shards,
    FileSystem* file_system)
    : file_prefix_(file_prefix),
      server_prefix_(server_prefix),
      num_shards_(num_shards),
      resource_id_(0),
      file_system_(file_system) {
}

FileResourceManager::~FileResourceManager() {
  for (size_t i = 0; i < resources_.size(); ++i) {
    delete resources_[i];
  }
}

// TODO(sligocki): combine this with CreateOutlineResource
SpriteResource* FileResourceManager::CreateSprite(const char* suffix) {
  std::string url(server_prefix_), filename(file_prefix_);
  int id = resource_id_++;
  char buf[100];
  snprintf(buf, sizeof(buf), "%d", id);
  url += buf;
  url += suffix;
  filename += buf;
  filename += suffix;
  SpriteResource* sprite = new SpriteResource(url, filename, id);
  resources_.push_back(sprite);
  return sprite;
}


// TODO(sligocki): don't pass in content here.
OutlineResource* FileResourceManager::CreateOutlineResource(
    const std::string& content, const char* suffix) {
  std::string url(server_prefix_), filename(file_prefix_);
  int id = resource_id_++;
  char buf[100];
  snprintf(buf, sizeof(buf), "%d", id);
  url += buf;
  url += suffix;
  filename += buf;
  filename += suffix;
  OutlineResource* resource = new OutlineResource(content, url, filename, id);
  resources_.push_back(resource);
  return resource;
}

// Creates the a resource object corresponding to the URL found in an
// href or link attribute.  This reference should either be a relative
// reference or an explicit "file://", otherwise NULL is returned.
Resource* FileResourceManager::CreateResource(const std::string& url) {
  FileResource* resource = NULL;

  // For the moment, we can only handle local file references.
  // TODO(jmarantz): use googleurl for this.
  if (strncmp(url.c_str(), kFileUrlPrefix, kFileUrlPrefixSize) == 0) {
    resource = new FileResource(url.c_str() + kFileUrlPrefixSize);
  } else {
    // Also handle relative references, for the moment assuming
    // that we will be reading these resources from the file system.
    bool is_relative = true;
    size_t colon = url.find(':');
    if (colon != std::string::npos) {
      size_t slash = url.find('/');
      if (colon < slash) {
        // Must be another protocol reference.
        is_relative = false;
      }
    }

    // the search path is not a bash-like :-separated path: just a single
    // file prefix that we use to locate resources in the file system.
    // TODO(jmarantz): rename this variable and associated methods.
    std::string filename(search_path_);
    if (!filename.empty()) {
      filename += '/';
    }
    filename += url;
    resource = new FileResource(filename.c_str());
  }
  resources_.push_back(resource);
  return resource;
}

bool FileResourceManager::WriteResource(
    Resource* resource, const char* filename, MessageHandler* message_handler) {
  FileSystem::OutputFile* f =
      file_system_->OpenOutputFile(filename, message_handler);
  FileWriter writer(f);
  bool ret = false;
  if (f != NULL) {
    ret = resource->Write(file_system_, &writer, message_handler);
    ret &= file_system_->Close(f, message_handler);
  }
  return ret;
}

bool FileResourceManager::Load(
    Resource* resource, MessageHandler* message_handler) {
  return resource->Load(file_system_, message_handler);
}
}
