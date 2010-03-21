// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "net/instaweb/htmlparse/public/stdio_file_system.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <string>
#include "net/instaweb/htmlparse/public/message_handler.h"

namespace net_instaweb {

namespace {
// Helper class to factor out common implementation details between Input and
// Output files, in lieu of multiple inheritance.
class FileHelper {
 public:
  FileHelper(FILE* f, const char* filename)
      : file_(f),
        filename_(filename),
        line_(1) {
  }

  ~FileHelper() {
    assert(file_ == NULL);
  }

  void CountNewlines(const char* buf, int size) {
    for (int i = 0; i < size; ++i, ++buf) {
      line_ += (*buf == '\n');
    }
  }

  void ReportError(MessageHandler* message_handler, const char* format) {
    message_handler->Error(filename_.c_str(), line_, format, strerror(errno));
  }

  bool Close(MessageHandler* message_handler) {
    bool ret = true;
    if (fclose(file_) != 0) {
      ReportError(message_handler, "closing file: %s");
      ret = false;
    }
    file_ = NULL;
    return ret;
  }

  FILE* file_;
  std::string filename_;
  int line_;
};
}

class StdioInputFile : public FileSystem::InputFile {
 public:
  StdioInputFile(FILE* f, const char* filename) : file_helper_(f, filename) {
  }

  virtual int Read(char* buf, int size, MessageHandler* message_handler) {
    int ret = fread(buf, 1, size, file_helper_.file_);
    file_helper_.CountNewlines(buf, ret);
    if ((ret == 0) && (ferror(file_helper_.file_) != 0)) {
      file_helper_.ReportError(message_handler, "reading file: %s");
    }
    return ret;
  }

  virtual bool Close(MessageHandler* message_handler) {
    return file_helper_.Close(message_handler);
  }

 private:
  FileHelper file_helper_;
};

class StdioOutputFile : public FileSystem::OutputFile {
 public:
  StdioOutputFile(FILE* f, const char* filename) : file_helper_(f, filename) {
  }

  virtual int Write(
      const char* buf, int size, MessageHandler* message_handler) {
    int ret = fwrite(buf, 1, size, file_helper_.file_);
    file_helper_.CountNewlines(buf, ret);
    if (ret != size) {
      file_helper_.ReportError(message_handler, "writing file: %s");
    }
    return ret;
  }

  virtual bool Flush(MessageHandler* message_handler) {
    bool ret = true;
    if (fflush(file_helper_.file_) != 0) {
      file_helper_.ReportError(message_handler, "flushing file: %s");
      ret = false;
    }
    return ret;
  }

  virtual bool Close(MessageHandler* message_handler) {
    return file_helper_.Close(message_handler);
  }

  virtual void SetWorldReadable() {
    fchmod(fileno(file_helper_.file_), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  }

 private:
  FileHelper file_helper_;
};

StdioFileSystem::~StdioFileSystem() {
}

FileSystem::InputFile* StdioFileSystem::OpenInputFile(
    const char* filename, MessageHandler* message_handler) {
  FileSystem::InputFile* input_file = NULL;
  FILE* f = fopen(filename, "r");
  if (f == NULL) {
    message_handler->Error(filename, 0, "opening file: %s", strerror(errno));
  } else {
    input_file = new StdioInputFile(f, filename);
  }
  return input_file;
}


FileSystem::OutputFile* StdioFileSystem::OpenOutputFile(
    const char* filename, MessageHandler* message_handler) {
  FileSystem::OutputFile* output_file = NULL;
  if (strcmp(filename, "-") == 0) {
    output_file = new StdioOutputFile(stdout, "<stdout>");
  } else {
    FILE* f = fopen(filename, "w");
    if (f == NULL) {
      message_handler->Error(filename, 0, "opening file: %s", strerror(errno));
    } else {
      output_file = new StdioOutputFile(f, filename);
    }
  }
  return output_file;
}
}
