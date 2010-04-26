// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_RESOURCE_SERVER_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_RESOURCE_SERVER_H_

#include <string>
#include "net/instaweb/util/public/string_util.h"

namespace net_instaweb {
class Hasher;
class MessageHandler;

// Manages serving of web resources (e.g. images, css, javascript).
class ResourceServer {
 public:
  ResourceServer(Hasher* hasher, MessageHandler* handler);

  // Generate a web-safe ID, suitable for inclusion in a URL, that
  // encodes a URL and a hash of its content.
  void EncodeResource(StringPiece url, const std::string& content,
                      std::string* url_safe_id);

  // Decode a web-safe ID, extracting the URL.
  bool DecodeResource(StringPiece url_safe_id, std::string* url);

 private:
  Hasher* hasher_;
  MessageHandler* message_handler_;
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_RESOURCE_SERVER_H_
