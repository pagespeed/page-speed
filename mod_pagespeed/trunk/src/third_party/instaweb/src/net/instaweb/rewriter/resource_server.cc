// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "net/instaweb/rewriter/public/resource_server.h"
#include "net/instaweb/rewriter/rewrite.pb.h"  // for ResourceUrl
#include "net/instaweb/util/public/hasher.h"
#include <string>
#include "net/instaweb/util/public/string_util.h"

namespace net_instaweb {

ResourceServer::ResourceServer(Hasher* hasher, MessageHandler* handler)
    : hasher_(hasher),
      message_handler_(handler) {
}

// TODO(jmarantz): consider creating a streaming interface to encoding a
// resource.  The resource may be very large, so we may not want to
// require a caller to collect it all in a std::string.
void ResourceServer::EncodeResource(
    StringPiece url, const std::string& content, std::string* url_safe_id) {
  // Construct a new URL for this resource that encodes the original
  // name and a hash of the resource content.
  ResourceUrl resource_url;
  resource_url.set_origin_url(url.data(), url.size());
  resource_url.set_content_hash(hasher_->Hash(content));
  std::string serialized_url;
  resource_url.SerializeToString(&serialized_url);
  std::string encoded_url;
  Web64Encode(serialized_url, url_safe_id);
}

bool ResourceServer::DecodeResource(StringPiece url_safe_id,
                                    std::string* url) {
  std::string decoded_resource;
  ResourceUrl resource_url;
  bool ret = false;
  if (Web64Decode(url_safe_id, &decoded_resource) &&
      resource_url.ParseFromString(decoded_resource)) {
    *url = resource_url.origin_url();
    ret = true;
  }
  return ret;
}

}  // namespace net_instaweb
