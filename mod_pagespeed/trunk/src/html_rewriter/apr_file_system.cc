// Copyright 2010 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "html_rewriter/apr_file_system.h"

#include <string>

#include "base/logging.h"
#include "net/instaweb/util/public/message_handler.h"
#include "third_party/apache_httpd/include/apr_file_io.h"
#include "third_party/apache_httpd/include/apr_pools.h"

using net_instaweb::MessageHandler;

namespace {
const int kErrorMessageBufferSize = 1024;
} // namespace

namespace html_rewriter {

// Helper class to factor out common implementation details between Input and
// Output files, in lieu of multiple inheritance.
class FileHelper {
 public:
  FileHelper(apr_file_t* file, const char* filename)
      : file_(file),
        filename_(filename) {
  }

  // Note: format must have "%d" and "%s" to correctly report the error code and
  // error message.
  void ReportError(MessageHandler* message_handler, const char* format,
                   int error_code) {
    char buf[kErrorMessageBufferSize];
    apr_strerror(error_code, buf, sizeof(buf));
    std::string error_format(format);
    error_format.append(" (code=%d %s)");
    message_handler->Error(filename_.c_str(), 0, error_format.c_str(),
                           error_code, buf);
  }

  bool Close(MessageHandler* message_handler);
  apr_file_t* file() { return file_; }
  const std::string& filename() const { return filename_; }

 private:
  apr_file_t* const file_;
  const std::string filename_;
};

bool FileHelper::Close(MessageHandler* message_handler) {
  apr_status_t ret = apr_file_close(file_);
  if (ret != APR_SUCCESS) {
    ReportError(message_handler, "close file", ret);
    return false;
  } else {
    return true;
  }
}

class HtmlWriterInputFile : public FileSystem::InputFile {
 public:
  HtmlWriterInputFile(apr_file_t* file, const char* filename);
  virtual int Read(char* buf, int size, MessageHandler* message_handler);
  virtual bool Close(MessageHandler* message_handler) {
    helper_.Close(message_handler);
  }
  virtual const char* filename() { return helper_.filename().c_str(); }
 private:
  FileHelper helper_;
};

class HtmlWriterOutputFile : public FileSystem::OutputFile {
 public:
  HtmlWriterOutputFile(apr_file_t* file, const char* filename);
  virtual int Write(const char* buf, int size,
                    MessageHandler* message_handler);
  virtual bool Flush(MessageHandler* message_handler);
  virtual bool Close(MessageHandler* message_handler) {
    helper_.Close(message_handler);
  }
  virtual bool SetWorldReadable(MessageHandler* message_handler);
  virtual const char* filename() { return helper_.filename().c_str(); }
 private:
  FileHelper helper_;
};

HtmlWriterInputFile::HtmlWriterInputFile(apr_file_t* file, const char* filename)
    : helper_(file, filename) {
}

int HtmlWriterInputFile::Read(char* buf,
                              int size,
                              MessageHandler* message_handler) {
  apr_size_t bytes = size;
  apr_status_t ret = apr_file_read(helper_.file(), buf, &bytes);
  if (ret != APR_SUCCESS) {
    bytes = -1;
    helper_.ReportError(message_handler, "read file", ret);
  }
  return bytes;
}

HtmlWriterOutputFile::HtmlWriterOutputFile(apr_file_t* file,
                                           const char* filename)
    : helper_(file, filename) {
}

int HtmlWriterOutputFile::Write(const char* buf,
                                int size,
                                MessageHandler* message_handler) {
  apr_size_t bytes = size;
  apr_status_t ret = apr_file_write(helper_.file(), buf, &bytes);
  if (ret != APR_SUCCESS) {
    bytes = -1;
    helper_.ReportError(message_handler, "write file", ret);
  }
  return bytes;
}

bool HtmlWriterOutputFile::Flush(MessageHandler* message_handler) {
  apr_status_t ret = apr_file_flush(helper_.file());
  if (ret != APR_SUCCESS) {
    helper_.ReportError(message_handler, "flush file", ret);
    return false;
  }
  return true;
}

bool HtmlWriterOutputFile::SetWorldReadable(MessageHandler* message_handler) {
  apr_status_t ret = apr_file_perms_set(helper_.filename().c_str(),
                         APR_FPROT_UREAD | APR_FPROT_UWRITE |
                         APR_FPROT_GREAD | APR_FPROT_WREAD);
  if (ret != APR_SUCCESS) {
    helper_.ReportError(message_handler, "set permission", ret);
    return false;
  }
  return true;
}

AprFileSystem::AprFileSystem(apr_pool_t* pool)
    : pool_(pool) {
}

AprFileSystem::~AprFileSystem() {
}

FileSystem::InputFile* AprFileSystem::OpenInputFile(
    const char* filename, MessageHandler* message_handler) {
  apr_file_t* file;
  apr_status_t ret = apr_file_open(&file, filename, APR_FOPEN_READ,
                                   APR_OS_DEFAULT, pool_);
  if (ret != APR_SUCCESS) {
    message_handler->Error(filename, 0, "open file", ret);
    return NULL;
  }
  return new HtmlWriterInputFile(file, filename);
}

// Expects the directories exist. The caller should create those directories
// before open the output file.
FileSystem::OutputFile* AprFileSystem::OpenOutputFile(
    const char* filename, MessageHandler* message_handler) {
  apr_file_t* file;
  apr_status_t ret = apr_file_open(&file, filename,
                                   APR_WRITE | APR_CREATE | APR_TRUNCATE,
                                   APR_OS_DEFAULT, pool_);
  if (ret != APR_SUCCESS) {
    message_handler->Error(filename, 0, "open file", ret);
    return NULL;
  }
  return new HtmlWriterOutputFile(file, filename);
}

}  // namespace html_rewriter
