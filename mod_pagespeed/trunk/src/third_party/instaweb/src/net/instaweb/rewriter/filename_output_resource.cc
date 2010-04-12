// Copyright 2010 and onwards Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)

#include "net/instaweb/rewriter/public/filename_output_resource.h"

#include <assert.h>
#include "net/instaweb/util/public/file_system.h"
#include "net/instaweb/util/public/simple_meta_data.h"

namespace net_instaweb {

FilenameOutputResource::FilenameOutputResource(const std::string& url,
                                               const std::string& filename,
                                               FileSystem* file_system)
  : url_(url),
    filename_(filename),
    file_system_(file_system),
    output_file_(NULL),
    metadata_(new SimpleMetaData),
    writing_complete_(false) {
}

FilenameOutputResource::~FilenameOutputResource() {
  delete metadata_;
}

bool FilenameOutputResource::StartWrite(MessageHandler* handler) {
  assert(!writing_complete_);
  assert(output_file_ == NULL);

  // Always write to a tempfile, so that if we get interrupted in the middle
  // we won't leave a half-baked file in the serving path.
  std::string temp_prefix = TempPrefix();
  output_file_ = file_system_->OpenTempFile(temp_prefix.c_str(), handler);
  return (output_file_ != NULL);
}

std::string FilenameOutputResource::TempPrefix() const {
  return filename_ + "_temp_";
}

bool FilenameOutputResource::WriteChunk(const char* buf, size_t size,
                                        MessageHandler* handler) {
  assert(!writing_complete_);
  assert(output_file_ != NULL);
  return output_file_->Write(buf, size, handler);
}

bool FilenameOutputResource::EndWrite(MessageHandler* handler) {
  assert(!writing_complete_);
  assert(output_file_ != NULL);
  writing_complete_ = true;
  std::string temp_filename = output_file_->filename();
  bool ret = file_system_->Close(output_file_, handler);

  // Now that we are doing writing, we can rename to the filename we
  // really want.
  if (ret) {
    ret = file_system_->RenameFile(temp_filename.c_str(), filename_.c_str(),
                                   handler);
  }

  output_file_ = NULL;
  return ret;
}

bool FilenameOutputResource::IsReadable() const {
  return writing_complete_;
}

const std::string& FilenameOutputResource::url() const {
  return url_;
}

const MetaData* FilenameOutputResource::metadata() const {
  return metadata_;
}
}
