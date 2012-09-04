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

#include "pagespeed/l10n/localizer.h"

#include <sstream>

#include "base/logging.h"
#include "pagespeed/formatters/formatter_util.h"

namespace {

const char* kNativeLocale = "en_US";

} // namespace

namespace pagespeed {

namespace l10n {

Localizer::~Localizer() {
}

const char* BasicLocalizer::GetLocale() const {
  return kNativeLocale;
}

bool BasicLocalizer::LocalizeString(const std::string& val,
                                    std::string* out) const {
  if (!out) {
    LOG(DFATAL) << "out == NULL";
    return false;
  }
  *out = val;
  return true;
}

bool BasicLocalizer::LocalizeInt(int64 val, std::string* out) const {
  if (!out) {
    LOG(DFATAL) << "out == NULL";
    return false;
  }
  std::ostringstream ss;
  ss << val;
  *out = ss.str();
  return true;
}

bool BasicLocalizer::LocalizeUrl(const std::string& url,
                                 std::string* out) const {
  if (!out) {
    LOG(DFATAL) << "out == NULL";
    return false;
  }
  *out = url;
  return true;
}

bool BasicLocalizer::LocalizeBytes(int64 bytes, std::string* out) const {
  if (!out) {
    LOG(DFATAL) << "out == NULL";
    return false;
  }
  *out = pagespeed::formatters::FormatBytes(bytes);
  return true;
}

bool BasicLocalizer::LocalizeTimeDuration(int64 ms, std::string* out) const {
  if (!out) {
    LOG(DFATAL) << "out == NULL";
    return false;
  }
  *out = pagespeed::formatters::FormatTimeDuration(ms);
  return true;
}

bool BasicLocalizer::LocalizePercentage(int64 p, std::string* out) const {
  if (!out) {
    LOG(DFATAL) << "out == NULL";
    return false;
  }
  std::ostringstream ss;
  ss << p << "%";
  *out = ss.str();
  return true;
}


const char* NullLocalizer::GetLocale() const {
  return kNativeLocale;
}

bool NullLocalizer::LocalizeString(const std::string& val,
                                   std::string* out) const {
  if (!out) {
    LOG(DFATAL) << "out == NULL";
    return false;
  }
  *out = val;
  return true;
}

bool NullLocalizer::LocalizeInt(int64 val, std::string* out) const {
  if (!out) {
    LOG(DFATAL) << "out == NULL";
    return false;
  }
  std::ostringstream ss;
  ss << val;
  *out = ss.str();
  return true;
}

bool NullLocalizer::LocalizeUrl(const std::string& url,
                                std::string* out) const {
  if (!out) {
    LOG(DFATAL) << "out == NULL";
    return false;
  }
  *out = url;
  return true;
}

bool NullLocalizer::LocalizeBytes(int64 bytes, std::string* out) const {
  if (!out) {
    LOG(DFATAL) << "out == NULL";
    return false;
  }
  std::ostringstream ss;
  ss << bytes;
  *out = ss.str();
  return true;
}

bool NullLocalizer::LocalizeTimeDuration(int64 ms, std::string* out) const {
  if (!out) {
    LOG(DFATAL) << "out == NULL";
    return false;
  }
  std::ostringstream ss;
  ss << ms;
  *out = ss.str();
  return true;
}

bool NullLocalizer::LocalizePercentage(int64 p, std::string* out) const {
  if (!out) {
    LOG(DFATAL) << "out == NULL";
    return false;
  }
  std::ostringstream ss;
  ss << p;
  *out = ss.str();
  return true;
}

} // namespace l10n

} // namespace pagespeed
