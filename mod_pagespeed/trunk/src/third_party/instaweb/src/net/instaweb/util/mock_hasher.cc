// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "net/instaweb/util/public/mock_hasher.h"
#include "net/instaweb/util/public/string_util.h"

namespace net_instaweb {

MockHasher::~MockHasher() {
}

void MockHasher::Reset() {
}

void MockHasher::Add(const StringPiece& content) {
}

void MockHasher::ComputeHash(std::string* hash) {
  *hash += '0';
}

}  // namespace net_instaweb
