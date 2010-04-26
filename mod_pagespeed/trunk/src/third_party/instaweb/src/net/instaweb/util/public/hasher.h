// Copyright 2010 and onwards Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)
//
// Interface for a hash function.

#ifndef NET_INSTAWEB_UTIL_PUBLIC_HASHER_H_
#define NET_INSTAWEB_UTIL_PUBLIC_HASHER_H_

#include <string>

namespace net_instaweb {

class Hasher {
 public:
  virtual ~Hasher();

  // Interface to compute a hash of a single string.
  std::string Hash(const std::string& content);

  // Interface to accummulate a hash of data.
  virtual void Reset() = 0;
  virtual void Add(const char* data, int size) = 0;
  virtual void ComputeHash(std::string* hash) = 0;
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_UTIL_PUBLIC_HASHER_H_
