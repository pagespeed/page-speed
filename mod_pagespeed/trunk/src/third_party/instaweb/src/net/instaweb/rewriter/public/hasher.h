// Copyright 2010 and onwards Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)
//
// Interface for a hash function.

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_HASHER_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_HASHER_H_

#include <string>

namespace net_instaweb {

class Hasher {
 public:
  virtual ~Hasher();

  virtual std::string Hash(const std::string& preimage) = 0;
};
}

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_HASHER_H_
