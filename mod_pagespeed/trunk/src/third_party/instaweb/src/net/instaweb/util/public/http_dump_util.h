// Copyright 2010 Google Inc. All Rights Reserved.
// Author: sligocki@google.com (Shawn Ligocki)
//
// Utilities used with latency lab's http dumps.
// Seperated from http_dump_url_fetcher to remove circular dependency.

#ifndef NET_INSTAWEB_UTIL_PUBLIC_HTTP_DUMP_UTIL_H_
#define NET_INSTAWEB_UTIL_PUBLIC_HTTP_DUMP_UTIL_H_

#include <string>

namespace net_instaweb {

class MessageHandler;
class MetaData;

namespace latencylab {

// Converts all non alphanumeric non-/ chars to xNN, where NN is the hex code.
void EscapeNonAlphanum(const std::string& in_word, std::string* out_word);

// Converts URL into filename the way that Latency Lab does.
// Note: root_dir_ must be standardized to have a / at end already.
bool GetFilenameFromUrl(const std::string& root_dir, const std::string& url,
                        std::string* filename,
                        MessageHandler* message_handler);

}  // namespace latencylab

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_UTIL_PUBLIC_HTTP_DUMP_UTIL_H_
