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

#include "pagespeed/l10n/gettext_localizer.h"

#include <iomanip>
#include <sstream>
#include <vector>

#include "base/logging.h"
#include "base/string_util.h"
#include "pagespeed/formatters/formatter_util.h"
#include "pagespeed/l10n/l10n.h"
#include "pagespeed/l10n/register_locale.h"

namespace pagespeed {

namespace l10n {

namespace {

// Constants used for formatting byte counts
const int kBytesPerKiB = 1 << 10;
const int kBytesPerMiB = 1 << 20;

} // namespace

GettextLocalizer* GettextLocalizer::Create(const std::string& locale) {
  const char** locale_table = RegisterLocale::GetStringTable(locale);
  if (!locale_table) {
    LOG(ERROR) << "could not find string table for locale '"
               << locale << "'";
    return NULL;
  }

  return new GettextLocalizer(locale_table);
}

GettextLocalizer::GettextLocalizer(const char** locale_string_table)
    : locale_string_table_(locale_string_table) {
}

bool GettextLocalizer::LocalizeString(const std::string& val,
                                      std::string* out) const {
  CHECK(out);

  const std::map<std::string, size_t>* master_string_map =
      RegisterLocale::GetMasterStringMap();
  CHECK(master_string_map);
  std::map<std::string, size_t>::const_iterator itr =
      master_string_map->find(val);

  // If the string isn't found in the table, then it was never extracted for
  // localization.
  if (itr == master_string_map->end()) {
    LOG(INFO) << "no entry in translation table for string '"
              << val << "'";
    *out = val;
    return false;
  }

  // If the translated string is empty, on the other hand, then it was extracted
  // for localization, but the localization hasn't happened yet.
  size_t idx = itr->second;
  if (locale_string_table_[idx][0] == '\0') {
    LOG(WARNING) << "no translation available for string '"
                 << val << "'";
    *out = val;
    return false;
  }

  *out = locale_string_table_[idx];
  return true;
}

bool GettextLocalizer::LocalizeInt(int64 val, std::string* out) const {
  CHECK(out);
  ClearOstream();
  ostream_ << val;
  *out = ostream_.str();
  return true;
}

bool GettextLocalizer::LocalizeUrl(const std::string& url,
                                   std::string* out) const {
  CHECK(out);
  *out = url;
  return true;
}

bool GettextLocalizer::LocalizeBytes(int64 bytes, std::string* out) const {
  CHECK(out);
  const char* format;
  std::string value;

  // TODO(aoates): correctly localize decimal places, etc.
  ClearOstream();
  if (bytes < kBytesPerKiB) {
    // TRANSLATOR: An amount of bytes with abbreviated unit.  $1 is a string
    // placeholder that is replaced with the number of bytes (e.g. "93",
    // representing 93 bytes).
    format = _("$1B");
    ostream_ << bytes;
    value = ostream_.str();
  } else if (bytes < kBytesPerMiB) {
    // TRANSLATOR: An amount of kilobytes with abbreviated unit.  $1 is a string
    // placeholder that is replaced with the number of kilobytes (e.g. "32.5",
    // representing 32.5 kilobytes).
    format = _("$1KiB");
    ostream_ << std::fixed << std::setprecision(1)
             << bytes / static_cast<double>(kBytesPerKiB);
    value = ostream_.str();
  } else {
    // TRANSLATOR: An amount of megabytes with abbreviated unit.  $1 is a string
    // placeholder that is replaced with the number of megabytes (e.g. "32.5",
    // representing 32.5 megabytes).
    format = _("$1MiB");
    ostream_ << std::fixed << std::setprecision(1)
             << bytes / static_cast<double>(kBytesPerMiB);
    value = ostream_.str();
  }

  std::string localized_format;
  bool success = LocalizeString(format, &localized_format);

  std::vector<std::string> subst;
  subst.push_back(value);
  *out = ReplaceStringPlaceholders(localized_format, subst, NULL);
  return success;
}

bool GettextLocalizer::LocalizeTimeDuration(int64 ms, std::string* out) const {
  CHECK(out);

  // TODO(aoates): localize time durations
  *out = pagespeed::formatters::FormatTimeDuration(ms);
  return false;
}

void GettextLocalizer::ClearOstream() const {
  ostream_.clear();
  ostream_.str("");
}

} // namespace l10n

} // namespace pagespeed
