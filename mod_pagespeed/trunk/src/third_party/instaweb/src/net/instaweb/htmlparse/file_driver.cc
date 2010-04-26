// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "net/instaweb/htmlparse/public/file_driver.h"
#include "net/instaweb/util/public/file_system.h"
#include "net/instaweb/util/public/file_writer.h"
#include "net/instaweb/htmlparse/public/file_statistics_log.h"
#include "net/instaweb/htmlparse/public/html_writer_filter.h"
#include "net/instaweb/util/public/message_handler.h"

namespace {

bool GenerateFilename(
    const char* extension, const bool keep_old_extension,
    const char* infilename, std::string* outfilename) {
  bool ret = false;
  const char* dot = strrchr(infilename, '.');
  if (dot != NULL) {
    outfilename->clear();
    int base_size = dot - infilename;
    outfilename->append(infilename, base_size);
    *outfilename += extension;
    if (keep_old_extension) {
      *outfilename += dot;
    }
    ret = true;
  }
  return ret;
}
}

namespace net_instaweb {

FileDriver::FileDriver(MessageHandler* message_handler,
                       FileSystem* file_system)
    : message_handler_(message_handler),
      html_parse_(message_handler_),
      logging_filter_(),
      html_write_filter_(&html_parse_),
      filters_added_(false),
      file_system_(file_system) {
}

bool FileDriver::GenerateOutputFilename(
    const char* infilename, std::string* outfilename) {
  return GenerateFilename(".out", true, infilename, outfilename);
}

bool FileDriver::GenerateStatsFilename(
    const char* infilename, std::string* outfilename) {
  return GenerateFilename(".stats", false, infilename, outfilename);
}

bool FileDriver::ParseFile(const char* infilename,
                           const char* outfilename,
                           const char* statsfilename) {
  MessageHandler* message_handler = html_parse_.message_handler();
  FileSystem::OutputFile* outf =
      file_system_->OpenOutputFile(outfilename, message_handler);
  bool ret = false;

  if (outf != NULL) {
    if (!filters_added_) {
      filters_added_ = true;
      html_parse_.AddFilter(&logging_filter_);
      html_parse_.AddFilter(&html_write_filter_);
    }
    logging_filter_.Reset();
    FileWriter file_writer(outf);
    html_write_filter_.set_writer(&file_writer);
    FileSystem::InputFile* f =
        file_system_->OpenInputFile(infilename, message_handler);
    if (f != NULL) {
      html_parse_.StartParse(infilename);
      char buf[1000];
      int nread;
      while ((nread = f->Read(buf, sizeof(buf), message_handler)) > 0) {
        html_parse_.ParseText(buf, nread);
      }
      file_system_->Close(f, message_handler);
      html_parse_.FinishParse();
      ret = true;
      if (statsfilename != NULL) {
        FileSystem::OutputFile* statsfile =
            file_system_->OpenOutputFile(statsfilename, message_handler);
        if (statsfile != NULL) {
          FileStatisticsLog statslog =
              FileStatisticsLog(statsfile, message_handler);
          logging_filter_.LogStatistics(&statslog);
          file_system_->Close(statsfile, message_handler);
        } else {
          ret = false;
        }
      }
    }
    file_system_->Close(outf, message_handler);
  }

  return ret;
}

}  // namespace net_instaweb
