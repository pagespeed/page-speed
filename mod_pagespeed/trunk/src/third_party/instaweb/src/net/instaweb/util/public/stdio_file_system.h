// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_UTIL_PUBLIC_STDIO_FILE_SYSTEM_H_
#define NET_INSTAWEB_UTIL_PUBLIC_STDIO_FILE_SYSTEM_H_

#include "net/instaweb/util/public/file_system.h"

namespace net_instaweb {

class StdioFileSystem : public FileSystem {
 public:
  StdioFileSystem() {}
  virtual ~StdioFileSystem();

  virtual InputFile* OpenInputFile(const char* filename,
                                   MessageHandler* message_handler);
  virtual OutputFile* OpenOutputFile(const char* filename,
                                     MessageHandler* message_handler);
  virtual OutputFile* OpenTempFile(const StringPiece& prefix_name,
                                   MessageHandler* message_handle);

  virtual bool RemoveFile(const char* filename, MessageHandler* handler);
  virtual bool RenameFile(const char* old_file, const char* new_file,
                          MessageHandler* handler);
  virtual bool MakeDir(const char* directory_path, MessageHandler* handler);
  virtual BoolOrError Exists(const char* path, MessageHandler* handler);
  virtual BoolOrError IsDir(const char* path, MessageHandler* handler);

 private:
  DISALLOW_COPY_AND_ASSIGN(StdioFileSystem);
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_UTIL_PUBLIC_STDIO_FILE_SYSTEM_H_
