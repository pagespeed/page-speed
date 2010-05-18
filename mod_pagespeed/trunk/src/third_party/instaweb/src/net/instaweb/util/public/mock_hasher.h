// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_UTIL_PUBLIC_MOCK_HASHER_H_
#define NET_INSTAWEB_UTIL_PUBLIC_MOCK_HASHER_H_

#include "net/instaweb/util/public/hasher.h"
#include <string>

namespace net_instaweb {

class MockHasher : public Hasher {
 public:
  MockHasher() {}
  ~MockHasher();
  virtual void Reset();
  virtual void Add(const StringPiece& content);
  virtual void ComputeHash(std::string* hash);
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_UTIL_PUBLIC_MOCK_HASHER_H_
