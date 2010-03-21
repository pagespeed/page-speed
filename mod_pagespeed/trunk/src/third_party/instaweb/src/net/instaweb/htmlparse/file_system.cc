// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "net/instaweb/htmlparse/public/file_system.h"

namespace {
// Size of stack buffer for read-blocks.  This can't be too big or it will blow
// the stack, which may be set small in multi-threaded environments.
const int kBufferSize = 10000;
}

namespace net_instaweb {

FileSystem::~FileSystem() {
}

FileSystem::File::~File() {
}

FileSystem::InputFile::~InputFile() {
}

FileSystem::OutputFile::~OutputFile() {
}

bool FileSystem::ReadFile(const char* filename, std::string* buffer,
                          MessageHandler* message_handler) {
  InputFile* input_file = OpenInputFile(filename, message_handler);
  bool ret = false;
  if (input_file != NULL) {
    char buf[kBufferSize];
    int nread;
    while ((nread = input_file->Read(buf, sizeof(buf), message_handler)) > 0) {
      buffer->append(buf, nread);
    }
    ret = (nread == 0);
    ret &= Close(input_file, message_handler);
  }
  return ret;
}

bool FileSystem::WriteFile(const char* filename, const std::string& buffer,
                           MessageHandler* message_handler) {
  OutputFile* output_file = OpenOutputFile(filename, message_handler);
  bool ret = false;
  if (output_file != NULL) {
    ret = output_file->Write(buffer.data(), buffer.size(), message_handler);
    ret &= Close(output_file, message_handler);
  }
  return ret;
}

bool FileSystem::Close(File* file, MessageHandler* message_handler) {
  bool ret = file->Close(message_handler);
  delete file;
  return ret;
}
}
