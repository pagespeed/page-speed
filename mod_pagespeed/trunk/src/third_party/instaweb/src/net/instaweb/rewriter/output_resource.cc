// Copyright 2010 and onwards Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)

#include "net/instaweb/rewriter/public/output_resource.h"

namespace net_instaweb {

OutputResource::~OutputResource() {
}

bool OutputResource::Write(const std::string& content,
                           MessageHandler* handler) {
  bool ret = StartWrite(handler);
  ret &= WriteChunk(content.data(), content.size(), handler);
  ret &= EndWrite(handler);
  return ret;
}

}  // namespace net_instaweb
