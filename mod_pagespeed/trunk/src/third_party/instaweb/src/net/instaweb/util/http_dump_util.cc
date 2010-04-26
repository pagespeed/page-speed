// Copyright 2010 Google Inc. All Rights Reserved.
// Author: sligocki@google.com (Shawn Ligocki)

#include "net/instaweb/util/public/http_dump_util.h"

#include <stdio.h>  // For snprintf

#include "net/instaweb/util/public/message_handler.h"
#include "net/instaweb/util/public/meta_data.h"
#include <string>

namespace net_instaweb {

namespace latencylab {

void EscapeNonAlphanum(const std::string& in_word, std::string* out_word) {
  char escaped_char[4];
  for (int i = 0, n = in_word.length(); i < n; ++i) {
    char c = in_word.at(i);
    if (isalnum(c) || c == '/') {
      out_word->append(1, c);
    } else {
      snprintf(escaped_char, sizeof(escaped_char), "x%02X", c);
      out_word->append(escaped_char);
    }
  }
}

// Note: root_dir_ must be standardized to have a / at end already.
bool GetFilenameFromUrl(const std::string& root_dir, const std::string& url,
                        std::string* filename,
                        MessageHandler* message_handler) {
  bool ret = false;

  // Seperate the url into domain and path.
  // TODO(sligocki): Use google_url.
  static const char kSchemeSeparator[] = "://";
  static const int kSchemeSeparatorLength = sizeof(kSchemeSeparator) - 1;
  size_t scheme_end = url.find(kSchemeSeparator);
  if (scheme_end != std::string::npos) {
    size_t domain_start = scheme_end + kSchemeSeparatorLength;
    size_t domain_end = url.find("/", domain_start);

    std::string domain, path;
    if (domain_end == std::string::npos) {
      domain = url.substr(domain_start, std::string::npos);
      path = "index.html";
      message_handler->Message(kError, "Url '%s' does not specify subpath. "
                                       "Defaulting to index.html", url.c_str());
    } else {
      domain = url.substr(domain_start, domain_end - domain_start);
      path = url.substr(domain_end + 1, std::string::npos);
    }

    std::string escaped_path;
    EscapeNonAlphanum(path, &escaped_path);
    // TODO(sligocki): Use StrCat or something to make efficient.
    *filename = root_dir;
    filename->append(domain);
    filename->append("/");
    filename->append(escaped_path);
    ret = true;
  }

  return ret;
}

}  // namespace latencylab

}  // namespace net_instaweb
