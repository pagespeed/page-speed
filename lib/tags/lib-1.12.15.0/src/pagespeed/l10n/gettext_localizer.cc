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
#include "pagespeed/core/string_util.h"
#include "pagespeed/formatters/formatter_util.h"
#include "pagespeed/l10n/l10n.h"
#include "pagespeed/l10n/register_locale.h"

namespace pagespeed {

namespace l10n {

namespace {

// Constants used for formatting byte counts
const int kBytesPerKiB = 1 << 10;
const int kBytesPerMiB = 1 << 20;

}  // namespace

void ParseLocaleString(const std::string& locale, std::string* language_out,
                       std::string* country_out, std::string* encoding_out) {
  size_t language_end = locale.find_first_of("_-@.");
  if (language_end == std::string::npos)
    language_end = locale.length();

  if (language_out)
    *language_out = locale.substr(0, language_end);

  if (language_end < locale.length()) {
    size_t country_end = locale.find_first_of("@.", language_end);
    if (country_end == std::string::npos)
      country_end = locale.length();

    if (country_out && (country_end - language_end) > 0) {
      *country_out = locale.substr(language_end + 1,
                                   country_end - language_end - 1);
    }

    // Strip unused @modifiers.
    if (country_end < locale.length()) {
      size_t encoding_end = locale.find('@', country_end);
      if (encoding_end == std::string::npos)
        encoding_end = locale.length();
      else
        LOG(INFO) << "ignoring unused @modifier in '" << locale << "'";

      if (encoding_out && (encoding_end - country_end) > 0) {
        *encoding_out = locale.substr(country_end + 1,
                                      encoding_end - country_end - 1);
      }
    }
  }
}

GettextLocalizer* GettextLocalizer::Create(const std::string& locale) {
  // Parse the locale string.
  std::string language, country, encoding;
  ParseLocaleString(locale, &language, &country, &encoding);

  // Check that encoding is empty or UTF-8.
  if (!encoding.empty() &&
      !pagespeed::string_util::StringCaseEqual(encoding, "utf-8")) {
    LOG(ERROR) << "could not provide encoding '" << encoding
               << "' for locale '" << locale << "'";
    return NULL;
  }

  std::string requested_locale;
  const char** locale_table = NULL;

  // Check <language>_<country> first.
  if (!country.empty()) {
    requested_locale = language + "_" + country;
    locale_table = RegisterLocale::GetStringTable(requested_locale);

    if (!locale_table) {
      LOG(INFO) << "could not find string table for locale '"
                << requested_locale << "', trying '" << language << "'";
    }
  }

  if (!locale_table) {
    requested_locale = language;
    locale_table = RegisterLocale::GetStringTable(requested_locale);
  }

  if (!locale_table) {
    LOG(ERROR) << "could not find string table matching locale '"
               << locale << "'";
    return NULL;
  } else {
    return new GettextLocalizer(requested_locale, locale_table);
  }
}

GettextLocalizer::GettextLocalizer(const std::string& locale,
                                   const char** locale_string_table)
    : locale_(locale), locale_string_table_(locale_string_table) {
}

const char* GettextLocalizer::GetLocale() const {
  return locale_.c_str();
}

bool GettextLocalizer::LocalizeString(const std::string& val,
                                      std::string* out) const {
  if (!out) {
    LOG(DFATAL) << "out == NULL";
    return false;
  }

  const std::map<std::string, size_t>* master_string_map =
      RegisterLocale::GetMasterStringMap();
  if (!master_string_map) {
    LOG(DFATAL) << "no master string table found";
    return false;
  }
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
  if (!out) {
    LOG(DFATAL) << "out == NULL";
    return false;
  }

  ClearOstream();
  ostream_ << val;
  *out = ostream_.str();
  return true;
}

bool GettextLocalizer::LocalizeUrl(const std::string& url,
                                   std::string* out) const {
  if (!out) {
    LOG(DFATAL) << "out == NULL";
    return false;
  }

  *out = url;
  return true;
}

bool GettextLocalizer::LocalizeBytes(int64 bytes, std::string* out) const {
  if (!out) {
    LOG(DFATAL) << "out == NULL";
    return false;
  }

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
  *out = pagespeed::string_util::ReplaceStringPlaceholders(
      localized_format, subst, NULL);
  return success;
}

bool GettextLocalizer::LocalizeTimeDuration(int64 ms, std::string* out) const {
  if (!out) {
    LOG(DFATAL) << "out == NULL";
    return false;
  }

  // TODO(aoates): localize time durations
  *out = pagespeed::formatters::FormatTimeDuration(ms);
  return false;
}

bool GettextLocalizer::LocalizePercentage(int64 p, std::string* out) const {
  if (!out) {
    LOG(DFATAL) << "out == NULL";
    return false;
  }

  // TODO(aoates): localize percentages
  ClearOstream();
  ostream_ << p << "%";
  *out = ostream_.str();
  return false;
}

void GettextLocalizer::ClearOstream() const {
  ostream_.clear();
  ostream_.str("");
}

}  // namespace l10n

}  // namespace pagespeed
