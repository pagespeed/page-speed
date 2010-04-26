// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_UTIL_PUBLIC_SYMBOL_TABLE_H_
#define NET_INSTAWEB_UTIL_PUBLIC_SYMBOL_TABLE_H_

#include <stdlib.h>
#include <string.h>
#include <set>
#include "net/instaweb/util/public/atom.h"
#include <string>
#include "net/instaweb/util/public/string_util.h"

namespace net_instaweb {

// Implements a generic symbol table, allowing for case-sensitive
// and case insensitive versions.  The elements of SymbolTables are
// Atoms.  Atoms are created by Interning strings.
//
// Atoms are cheap and are passed around by value, not by reference or
// pointer.  Atoms can be compared to one another via ==.  A const char*
// can be extracted from an Atom via ==.
//
// Atoms are memory-managed by the symbol table from which they came.
// When the symbol table is destroyed, so are all the Atoms that
// were interned in it.
//
// Care should be taken not to attempt to compare Atoms created from
// multiple symbol tables.
//
// TODO(jmarantz): Symbol tables are not currently thread-safe.  We
// should consider whether it's worth making them thread-safe, or
// whether it's better to use separate symbol tables in each thread.
template<class SymbolCompare> class SymbolTable {
 public:
  ~SymbolTable() {
    while (!string_set_.empty()) {
      // Note: This should perform OK for rb-trees, but will perform
      // poorly if a hash-table is used.
      typename StringSet::iterator p = string_set_.begin();
      char* str = *p;
      string_set_.erase(p);
      free(str);
    }
  }

  Atom Intern(const char* src) {
    Atom atom(src);
    typename StringSet::iterator iter =
        string_set_.find(const_cast<char*>(src));
    if (iter == string_set_.end()) {
      char* str = strdup(src);
      string_set_.insert(str);
      return Atom(str);
    }
    return Atom(*iter);
  }

  inline Atom Intern(const std::string& src) {
    return Intern(src.c_str());
  }

 private:
  typedef std::set<char*, SymbolCompare> StringSet;
  StringSet string_set_;
};

class SymbolTableInsensitive : public SymbolTable<CharStarCompareInsensitive> {
};
class SymbolTableSensitive : public SymbolTable<CharStarCompareSensitive> {
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_UTIL_PUBLIC_SYMBOL_TABLE_H_
