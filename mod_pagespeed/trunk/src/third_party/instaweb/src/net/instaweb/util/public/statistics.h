// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_UTIL_PUBLIC_STATISTICS_H_
#define NET_INSTAWEB_UTIL_PUBLIC_STATISTICS_H_

#include "net/instaweb/util/public/string_util.h"

namespace net_instaweb {

class Variable {
 public:
  virtual ~Variable();
  virtual StringPiece name() const = 0;
  virtual int Get() const = 0;
  virtual void Set(int delta) = 0;

  void Add(int delta) { Set(delta + Get()); }
  void Clear() { Set(0); }
};

// Helps build a statistics that can be exported as a CSV file.
class Statistics {
 public:
  virtual ~Statistics();

  // Add a new variable, or returns an existing one of that name.
  // The Variable* is owned by the Statistics class -- it should
  // not be deleted by the caller.
  virtual Variable* AddVariable(const StringPiece& name) = 0;

  // Find a variable from a name, returning NULL if not found.
  virtual Variable* FindVariable(const StringPiece& name) const = 0;
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_UTIL_PUBLIC_STATISTICS_H_
