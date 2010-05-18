// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_UTIL_PUBLIC_FILE_SYSTEM_H_
#define NET_INSTAWEB_UTIL_PUBLIC_FILE_SYSTEM_H_

#include <string>
#include "net/instaweb/util/public/string_util.h"

namespace net_instaweb {

// Three-way return type for distinguishing Errors from boolean answer.
//
// This is physically just an enum, but is wrapped in a class to prevent
// accidental usage in an if- or ternary-condition without explicitly indicating
// whether you are looking for true, false, or error.
class BoolOrError {
  enum Choice {
    kIsFalse,
    kIsTrue,
    kIsError
  };

 public:
  BoolOrError() : choice_(kIsError) { }
  explicit BoolOrError(bool t_or_f) : choice_(t_or_f ? kIsTrue : kIsFalse) { }
  bool is_false() const { return choice_ == kIsFalse; }
  bool is_true() const { return choice_ == kIsTrue; }
  bool is_error() const { return choice_ == kIsError; }
  void set_error() { choice_ = kIsError; }
  void set(bool t_or_f) { choice_ = t_or_f ? kIsTrue : kIsFalse; }

 private:
  Choice choice_;
};

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
    virtual bool Write(const StringPiece& buf, MessageHandler* handler) = 0;
    virtual bool Flush(MessageHandler* message_handler) = 0;
    virtual bool SetWorldReadable(MessageHandler* message_handler) = 0;

   protected:
    friend class FileSystem;
    virtual ~OutputFile();
  };

  // High level support to read/write entire files in one shot.
  virtual bool ReadFile(const char* filename,
                        std::string* buffer,
                        MessageHandler* message_handler);
  virtual bool WriteFile(const char* filename,
                         const StringPiece& buffer,
                         MessageHandler* message_handler);
  // Writes given data to a temp file in one shot, storing the filename
  // in filename on success.  Returns false and clears filename on failure.
  virtual bool WriteTempFile(const StringPiece& prefix_name,
                             const StringPiece& buffer,
                             std::string* filename,
                             MessageHandler* message_handler);

  virtual InputFile* OpenInputFile(const char* filename,
                                   MessageHandler* message_handler) = 0;
  virtual OutputFile* OpenOutputFile(const char* filename,
                                     MessageHandler* message_handler) = 0;
  // Opens a temporary file to write, with the specified prefix.
  // If successful, the filename can be obtained from File::filename().
  //
  // NULL is returned on failure.
  virtual OutputFile* OpenTempFile(const StringPiece& prefix_name,
                                   MessageHandler* message_handle) = 0;

  // Closes the File and cleans up memory.
  virtual bool Close(File* file, MessageHandler* message_handler);


  // Like POSIX 'rm'.
  virtual bool RemoveFile(const char* filename, MessageHandler* handler) = 0;

  // Like POSIX 'mv'.
  virtual bool RenameFile(const char* old_filename, const char* new_filename,
                          MessageHandler* handler) = 0;

  // Like POSIX 'mkdir', makes a directory only if parent directory exists.
  // Fails if directory_name already exists or parent directory doesn't exist.
  virtual bool MakeDir(const char* directory_path, MessageHandler* handler) = 0;

  // Like POSIX 'text -e', checks if path exists (is a file, directory, etc.).
  virtual BoolOrError Exists(const char* path, MessageHandler* handler) = 0;

  // Like POSIX 'test -d', checks if path exists and refers to a directory.
  virtual BoolOrError IsDir(const char* path, MessageHandler* handler) = 0;

  // Like POSIX 'mkdir -p', makes all directories up to this one recursively.
  // Fails if we do not have permission to make any directory in chain.
  virtual bool RecursivelyMakeDir(const StringPiece& directory_path,
                                  MessageHandler* handler);
};

// Make sure directory's path ends in '/'
void EnsureEndsInSlash(std::string* directory);

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_UTIL_PUBLIC_FILE_SYSTEM_H_
