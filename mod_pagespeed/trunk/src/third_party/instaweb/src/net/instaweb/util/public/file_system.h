// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_UTIL_PUBLIC_FILE_SYSTEM_H_
#define NET_INSTAWEB_UTIL_PUBLIC_FILE_SYSTEM_H_

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

    // Gets the name of the file.
    virtual const char* filename() = 0;

   protected:
    // Use public interface provided by FileSystem::Close.
    friend class FileSystem;
    virtual bool Close(MessageHandler* message_handler) = 0;
  };

  class InputFile : public File {
   public:
    // TODO(sligocki): Perhaps this should be renamed to avoid confusing
    // that it returns a bool to indicate success like all other Read methods
    // in our codebase.
    virtual int Read(char* buf, int size, MessageHandler* message_handler) = 0;

   protected:
    friend class FileSystem;
    virtual ~InputFile();
  };

  class OutputFile : public File {
   public:
    // Note: Write is not atomic. If Write fails, there is no indication of how
    // much data has already been written to the file.
    //
    // TODO(sligocki): Would we like a version that returns the amound written?
    // If so, it should be named so that it is clear it is returning int.
    virtual bool Write(const char* buf, int bytes, MessageHandler* handler) = 0;
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

  // Opens a temporary file to write, with the specified prefix.
  // If successful, the filename can be obtained from File::filename().
  //
  // NULL is returned on failure.
  virtual OutputFile* OpenTempFile(const char* prefix_name,
                                   MessageHandler* message_handle) = 0;

  // Renames a file
  virtual bool RenameFile(const char* old_filename, const char* new_filename,
                          MessageHandler* handler) = 0;
  // Delete a file
  virtual bool RemoveFile(const char* filename,
                          MessageHandler* handler) = 0;

};

// Make sure directory's path ends in '/'
void StandardizePath(std::string* directory);

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_UTIL_PUBLIC_FILE_SYSTEM_H_
