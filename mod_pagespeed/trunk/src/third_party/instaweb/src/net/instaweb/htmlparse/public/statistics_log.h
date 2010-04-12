// Copyright 2010 Google Inc. All Rights Reserved.
// Author: jmaessen@google.com (Jan Maessen)

#ifndef NET_INSTAWEB_HTMLPARSE_PUBLIC_STATISTICS_LOG_H_
#define NET_INSTAWEB_HTMLPARSE_PUBLIC_STATISTICS_LOG_H_

namespace net_instaweb {
class StatisticsLog {
 public:
  virtual ~StatisticsLog();
  virtual void LogStat(const char *statName, int value) = 0;
  virtual void LogDifference(const char *statName,
                             int value1, int value2) = 0;
};
}

#endif  // NET_INSTAWEB_HTMLPARSE_PUBLIC_STATISTICS_LOG_H_
