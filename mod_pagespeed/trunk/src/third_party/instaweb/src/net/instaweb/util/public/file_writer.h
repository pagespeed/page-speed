// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_UTIL_PUBLIC_FILE_WRITER_H_
#define NET_INSTAWEB_UTIL_PUBLIC_FILE_WRITER_H_

#include "net/instaweb/util/public/file_system.h"
#include "net/instaweb/util/public/writer.h"

namespace net_instaweb {

// Writer implementation for directing HTML output to a file.
class FileWriter : public Writer {
 public:
  explicit FileWriter(FileSystem::OutputFile* f) : file_(f) { }
  virtual ~FileWriter();
  virtual bool Write(const char* str, int len, MessageHandler* message_handler);
  virtual bool Flush(MessageHandler* message_handler);
 private:
  FileSystem::OutputFile* file_;
};
}

#endif  // NET_INSTAWEB_UTIL_PUBLIC_FILE_WRITER_H_
