// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_UTIL_PUBLIC_STDIO_FILE_SYSTEM_H_
#define NET_INSTAWEB_UTIL_PUBLIC_STDIO_FILE_SYSTEM_H_

#include "net/instaweb/util/public/file_system.h"

namespace net_instaweb {

class StdioFileSystem : public FileSystem {
 public:
  virtual ~StdioFileSystem();

  virtual InputFile* OpenInputFile(
      const char* file, MessageHandler* message_handler);
  virtual OutputFile* OpenOutputFile(
      const char* file, MessageHandler* message_handler);
  virtual OutputFile* OpenTempFile(const char* prefix,
                                   MessageHandler* message_handle);
  virtual bool RenameFile(const char* old_file, const char* new_file,
                          MessageHandler* message_handler);
  virtual bool RemoveFile(const char* filename,
                          MessageHandler* handler);
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_UTIL_PUBLIC_STDIO_FILE_SYSTEM_H_
