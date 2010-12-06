// Copyright 2010 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// Author: aoates@google.com (Andrew Oates)

#ifndef PAGESPEED_L10N_LOCALIZER_H_
#define PAGESPEED_L10N_LOCALIZER_H_

#include <string>

#include "base/basictypes.h"

namespace pagespeed {

namespace l10n {

/**
 * Interface for a class that provides localization.
 *
 * Provides methods for localizing generic constants (such as strings and ints),
 * as well as converting values such as byte counts and time durations into a
 * localized, human-readable format.
 */
class Localizer {
 public:
  virtual ~Localizer();

  // Localize a string constant into the current locale
  virtual std::string LocalizeString(const std::string& val) const = 0;

  // Localize an integer constant into the current locale
  virtual std::string LocalizeInt(int64 val) const = 0;

  // Localize a URL into the current locale
  virtual std::string LocalizeUrl(const std::string& url) const = 0;

  // Localize a byte count into a human-readable string in the current locale
  virtual std::string LocalizeBytes(int64 bytes) const = 0;

  // Localize a time duration (in ms) into a human-readable string in the
  // current locale
  virtual std::string LocalizeTimeDuration(int64 ms) const = 0;
};

/**
 * A localizer that localizes to English (doesn't change constants, and
 * humanizes byte counts and time durations in English).
 */
class BasicLocalizer : public Localizer {
 public:
  virtual std::string LocalizeString(const std::string& val) const;
  virtual std::string LocalizeInt(int64 val) const;
  virtual std::string LocalizeUrl(const std::string& url) const;
  virtual std::string LocalizeBytes(int64 bytes) const;
  virtual std::string LocalizeTimeDuration(int64 ms) const;
};

/**
 * A localizer that does nothing (just converts all values into strings).
 */
class NullLocalizer : public Localizer {
 public:
  virtual std::string LocalizeString(const std::string& val) const;
  virtual std::string LocalizeInt(int64 val) const;
  virtual std::string LocalizeUrl(const std::string& url) const;
  virtual std::string LocalizeBytes(int64 bytes) const;
  virtual std::string LocalizeTimeDuration(int64 ms) const;
};

} // namespace l10n

} // namespace pagespeed

#endif  // PAGESPEED_L10N_LOCALIZER_H_
