// Copyright 2010 Google Inc. All Rights Reserved.
// Author: jmaessen@google.com (Jan Maessen)

#include "net/instaweb/htmlparse/public/file_statistics_log.h"

#include <stdio.h>
#include "net/instaweb/util/public/file_system.h"
#include "net/instaweb/util/public/message_handler.h"
#include <string>
#include "net/instaweb/util/public/string_util.h"

// TODO(jmarantz): convert to statistics interface

namespace net_instaweb {

FileStatisticsLog::FileStatisticsLog(FileSystem::OutputFile* file,
                                     MessageHandler* message_handler)
  : file_(file),
    message_handler_(message_handler) {
}

FileStatisticsLog::~FileStatisticsLog() {
}

void FileStatisticsLog::LogStat(const char *stat_name, int value) {
  // Buffer whole log entry before writing, in case there's interleaving going
  // on (ie avoid multiple writes for single log entry)
  std::string buf(stat_name);
  buf += StrCat(": ", IntegerToString(value), "\n");
  file_->Write(buf, message_handler_);
}

void FileStatisticsLog::LogDifference(const char *stat_name,
                                      int value1, int value2) {
  // Buffer whole log entry before writing, in case there's interleaving going
  // on (ie avoid multiple writes for single log entry)
  std::string buf(stat_name);
  buf += StrCat(":\t", IntegerToString(value1), " vs\t",
                IntegerToString(value2));
  buf += StrCat("\tdiffer by\t", IntegerToString(value1 - value2), "\n");
  file_->Write(buf, message_handler_);
}

}  // namespace net_instaweb
