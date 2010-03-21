// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_HTMLPARSE_PUBLIC_STDIO_FILE_SYSTEM_H_
#define NET_INSTAWEB_HTMLPARSE_PUBLIC_STDIO_FILE_SYSTEM_H_

#include "net/instaweb/htmlparse/public/file_system.h"

namespace net_instaweb {
class StdioFileSystem : public FileSystem {
 public:
  virtual ~StdioFileSystem();

  virtual InputFile* OpenInputFile(
      const char* file, MessageHandler* message_handler);
  virtual OutputFile* OpenOutputFile(
      const char* file, MessageHandler* message_handler);
};
}

#endif  // NET_INSTAWEB_HTMLPARSE_PUBLIC_STDIO_FILE_SYSTEM_H_
