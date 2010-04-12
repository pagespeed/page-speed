// Copyright 2010 Google Inc. All Rights Reserved.
// Author: jmaessen@google.com (Jan Maessen)

#ifndef NET_INSTAWEB_HTMLPARSE_PUBLIC_FILE_STATISTICS_LOG_H_
#define NET_INSTAWEB_HTMLPARSE_PUBLIC_FILE_STATISTICS_LOG_H_

#include "net/instaweb/htmlparse/public/statistics_log.h"
#include "net/instaweb/util/public/file_system.h"

namespace net_instaweb {

class MessageHandler;

// Statistics logger that sends its output to a file.
class FileStatisticsLog : public StatisticsLog {
 public:
  // Note: calling context responsible for closing & cleaning up file.
  explicit FileStatisticsLog(FileSystem::OutputFile* file,
                             MessageHandler* message_handler);
  virtual ~FileStatisticsLog();
  virtual void LogStat(const char *statName, int value);
  virtual void LogDifference(const char *statName,
                             int value1, int value2);
 private:
  FileSystem::OutputFile* file_;
  MessageHandler* message_handler_;
};
}

#endif  // NET_INSTAWEB_HTMLPARSE_PUBLIC_FILE_STATISTICS_LOG_H_
