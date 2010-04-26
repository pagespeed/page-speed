// Copyright 2010 and onwards Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_FILENAME_OUTPUT_RESOURCE_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_FILENAME_OUTPUT_RESOURCE_H_

#include "net/instaweb/rewriter/public/output_resource.h"
#include "net/instaweb/util/public/file_system.h"
#include "net/instaweb/util/public/simple_meta_data.h"
#include <string>

namespace net_instaweb {

class MessageHandler;

class FilenameOutputResource : public OutputResource {
 public:
  FilenameOutputResource(const std::string& url,
                         const std::string& filename,
                         const bool write_http_headers,
                         FileSystem* file_system);
  virtual ~FilenameOutputResource();

  // Start a writing an output resource.
  virtual bool StartWrite(MessageHandler* message_handler);
  virtual bool WriteChunk(const char* buf, size_t size,
                          MessageHandler* message_handler);
  virtual bool EndWrite(MessageHandler* message_handler);

  virtual bool IsReadable() const;

  virtual const std::string& url() const { return url_; }
  virtual const MetaData* metadata() const { return &metadata_; }
  virtual MetaData* metadata() { return &metadata_; }

 protected:
  virtual std::string TempPrefix() const;

  std::string url_;
  std::string filename_;
  bool write_http_headers_;
  FileSystem* file_system_;
  FileSystem::OutputFile* output_file_;
  SimpleMetaData metadata_;
  bool writing_complete_;
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_FILENAME_OUTPUT_RESOURCE_H_
