// Copyright 2010 Google Inc. All Rights Reserved.
// Author: aoates@google.com (Andrew Oates)

#ifndef PAGESPEED_L10N_LOCALIZED_STRING_H_
#define PAGESPEED_L10N_LOCALIZED_STRING_H_

#include <string>

namespace pagespeed {

// A localizable string (a string for which a translation has been generated).
// All functions and methods that generate user-facing strings take a
// LocalizableString, which are only created when a string is passed through the
// _(...) localization markup macro in l10n.h.  This allows checking at
// compile-time that all user-facing strings are appropriately marked.
class LocalizableString {
 public:
  LocalizableString() : value_(NULL) {}
  // This should NEVER be called, except by the macros in l10n.h
  explicit LocalizableString(const char* s) : value_(s) {}

  // Conversion operator that allows LocalizableString to be cast into a
  // const char* (e.g. for use passing to a method that takes a const char*)
  operator const char*() const { return value_; }

 private:
  const char* value_;
};

} // namespace pagespeed

#endif  // PAGESPEED_L10N_LOCALIZED_STRING_H_
