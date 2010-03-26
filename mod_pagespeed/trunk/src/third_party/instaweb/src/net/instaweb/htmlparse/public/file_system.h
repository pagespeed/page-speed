// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_HTMLPARSE_PUBLIC_FILE_SYSTEM_H_
#define NET_INSTAWEB_HTMLPARSE_PUBLIC_FILE_SYSTEM_H_

#include <string>

namespace net_instaweb {

class MessageHandler;

// Provides abstract file system interface.  This isolation layer helps us:
//   - write unit tests that don't test the physical filesystem via a
//     MemFileSystem.
//   - Eases integration with Apache, which has its own file system interface,
//     and this class can help serve as the glue.
//   - provides a speculative conduit to a database so we can store resources
//     in a place where multiple Apache servers can see them.
class FileSystem {
 public:
  virtual ~FileSystem();

  class File {
   public:
    virtual ~File();
   protected:
    // Use public interface provided by FileSystem::Close.
    friend class FileSystem;
    virtual bool Close(MessageHandler* message_handler) = 0;
  };

  class InputFile : public File {
   public:
    virtual int Read(char* buf, int size, MessageHandler* message_handler) = 0;
   protected:
    friend class FileSystem;
    virtual ~InputFile();
  };

  class OutputFile : public File {
   public:
    virtual int Write(
        const char* buf, int bytes, MessageHandler* message_handler) = 0;
    virtual bool Flush(MessageHandler* message_handler) = 0;
    virtual bool SetWorldReadable(MessageHandler* message_handler) = 0;
   protected:
    friend class FileSystem;
    virtual ~OutputFile();
  };

  // High level support to read/write entire files in one shot
  virtual bool ReadFile(const char* filename, std::string* buffer,
                        MessageHandler* message_handler);
  virtual bool WriteFile(const char* filename, const std::string& buffer,
                         MessageHandler* message_handler);

  virtual InputFile* OpenInputFile(
      const char* file, MessageHandler* message_handler) = 0;
  virtual OutputFile* OpenOutputFile(
      const char* file, MessageHandler* message_handler) = 0;

  // Closes the File and deletes it.
  virtual bool Close(File* file, MessageHandler* message_handler);
};
}

#endif  // NET_INSTAWEB_HTMLPARSE_PUBLIC_FILE_SYSTEM_H_
