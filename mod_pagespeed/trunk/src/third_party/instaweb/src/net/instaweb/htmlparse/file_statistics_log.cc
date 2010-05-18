// Copyright 2010 Google Inc. All Rights Reserved.
// Author: jmaessen@google.com (Jan Maessen)

#include "net/instaweb/htmlparse/public/file_statistics_log.h"

#include <stdio.h>
#include "net/instaweb/util/public/file_system.h"
#include "net/instaweb/util/public/message_handler.h"
#include <string>

namespace {
void AppendInt(int i, std::string *buf) {
  char nbuf[100];
  snprintf(nbuf, sizeof(nbuf), "%d", i);
  *buf += nbuf;
}
}  // namespace

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
  buf += ": ";
  AppendInt(value, &buf);
  buf += "\n";
  file_->Write(buf, message_handler_);
}

void FileStatisticsLog::LogDifference(const char *stat_name,
                                      int value1, int value2) {
  // Buffer whole log entry before writing, in case there's interleaving going
  // on (ie avoid multiple writes for single log entry)
  std::string buf(stat_name);
  buf += ":\t";
  AppendInt(value1, &buf);
  buf += " vs\t";
  AppendInt(value2, &buf);
  buf += "\tdiffer by\t";
  AppendInt(value1-value2, &buf);
  buf += "\n";
  file_->Write(buf, message_handler_);
}

}  // namespace net_instaweb
