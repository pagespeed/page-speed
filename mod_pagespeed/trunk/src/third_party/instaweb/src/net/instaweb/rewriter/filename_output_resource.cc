// Copyright 2010 and onwards Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)

#include "net/instaweb/rewriter/public/filename_output_resource.h"

#include <assert.h>
#include "net/instaweb/util/public/file_system.h"
#include "net/instaweb/util/public/simple_meta_data.h"
#include "net/instaweb/util/public/string_util.h"
#include "net/instaweb/util/public/string_writer.h"

namespace net_instaweb {

FilenameOutputResource::FilenameOutputResource(const StringPiece& url,
                                               const StringPiece& filename,
                                               const bool write_http_headers,
                                               FileSystem* file_system)
  : write_http_headers_(write_http_headers),
    file_system_(file_system),
    output_file_(NULL),
    writing_complete_(false) {
  url.CopyToString(&url_);
  filename.CopyToString(&filename_);
}

FilenameOutputResource::~FilenameOutputResource() {
}

bool FilenameOutputResource::StartWrite(MessageHandler* handler) {
  assert(!writing_complete_);
  assert(output_file_ == NULL);

  // Always write to a tempfile, so that if we get interrupted in the middle
  // we won't leave a half-baked file in the serving path.
  std::string temp_prefix = TempPrefix();
  output_file_ = file_system_->OpenTempFile(temp_prefix.c_str(), handler);
  bool success = (output_file_ != NULL);
  if (write_http_headers_ && success) {
    std::string header;
    StringWriter writer(&header);
    metadata_.Write(&writer, handler);  // Serialize header.

    // Don't call WriteChunk, which is overridden by HashResourceManager.
    // It does not make sense to have the headers in the hash.  Instead,
    // call output_file_->Write directly.
    //
    // TODO(jmarantz): consider refactoring to split out the header-file
    // writing in a different way, e.g. to a separate file.
    success &= output_file_->Write(header, handler);
  }
  return success;
}

std::string FilenameOutputResource::TempPrefix() const {
  return filename_ + "_temp_";
}

bool FilenameOutputResource::WriteChunk(const StringPiece& buf,
                                        MessageHandler* handler) {
  assert(!writing_complete_);
  assert(output_file_ != NULL);
  return output_file_->Write(buf, handler);
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

// Resources stored in a file system are readable as soon as they are written.
// But if we were to store resources in a CDN with a 1 minute push process, then
// it's possible that IsReadable might lag IsWritten.
bool FilenameOutputResource::IsReadable() const {
  return writing_complete_;
}

bool FilenameOutputResource::IsWritten() const {
  return writing_complete_;
}

}  // namespace net_instaweb
