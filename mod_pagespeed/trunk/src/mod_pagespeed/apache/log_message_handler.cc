// Copyright 2010 Google Inc.
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

// Copied from mod_spdy
#include "mod_pagespeed/apache/log_message_handler.h"

#include "base/debug_util.h"
#include "base/logging.h"
#include "mod_pagespeed/apache/pool_util.h"
#include "third_party/apache_httpd/include/httpd.h"
#include "third_party/apache_httpd/include/http_log.h"

// Make sure we don't attempt to use LOG macros here, since doing so
// would cause us to go into an infinite log loop.
#undef LOG
#define LOG USING_LOG_HERE_WOULD_CAUSE_INFINITE_RECURSION

namespace {

int GetApacheLogLevel(int severity) {
  switch (severity) {
    case logging::LOG_INFO:
      return APLOG_NOTICE;
    case logging::LOG_WARNING:
      return APLOG_WARNING;
    case logging::LOG_ERROR:
      return APLOG_ERR;
    case logging::LOG_ERROR_REPORT:
      return APLOG_CRIT;
    case logging::LOG_FATAL:
      return APLOG_ALERT;
    default:
      return APLOG_NOTICE;
  }
}

bool LogMessageHandler(int severity,
                       const std::string& str) {
  const int log_level = GetApacheLogLevel(severity);

#ifdef NDEBUG
  // In release builds, don't log unless it's high priority (just
  // silently consume the log message).
  if (log_level != APLOG_ERR &&
      log_level != APLOG_CRIT &&
      log_level != APLOG_ALERT &&
      log_level != APLOG_EMERG) {
    return true;
  }
#endif

  std::string message = str;
  if (severity == logging::LOG_FATAL) {
    if (DebugUtil::BeingDebugged()) {
      DebugUtil::BreakDebugger();
    } else {
#ifndef NDEBUG
      // In debug, dump a stack trace on a fatal.
      StackTrace trace;
      std::ostringstream stream;
      trace.OutputToStream(&stream);
      message.append(stream.str());
#endif
    }
  }

  // Trim the newline off the end of the message string.
  size_t last_msg_character_index = message.length() - 1;
  if (last_msg_character_index > -1 &&
      message[last_msg_character_index] == '\n') {
    message.resize(last_msg_character_index);
  }
  mod_pagespeed::LocalPool local_pool;
  if (local_pool.status() == APR_SUCCESS) {
    ap_log_perror(APLOG_MARK,
                  log_level,
                  APR_SUCCESS,
                  local_pool.pool(),
                  "%s", message.c_str());
  } else {
    fprintf(stderr,
            "ap_log_perror failed. dumping to console: \n%s", message.c_str());
  }

  if (severity == logging::LOG_FATAL) {
    // Crash the process to generate a dump.
    DebugUtil::BreakDebugger();
  }

  return true;
}

}  // namespace

namespace mod_pagespeed {

void InstallLogMessageHandler() {
  logging::SetLogMessageHandler(&LogMessageHandler);
}

}  // namespace mod_pagespeed
