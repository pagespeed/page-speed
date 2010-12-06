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

namespace pagespeed {

namespace l10n {

Localizer::~Localizer() {
}


std::string BasicLocalizer::LocalizeString(const std::string& val) const {
  return val;
}

std::string BasicLocalizer::LocalizeInt(int64 val) const {
  std::ostringstream ss;
  ss << val;
  return ss.str();
}

std::string BasicLocalizer::LocalizeUrl(const std::string& url) const {
  return url;
}

std::string BasicLocalizer::LocalizeBytes(int64 bytes) const {
  return pagespeed::formatters::FormatBytes(bytes);
}

std::string BasicLocalizer::LocalizeTimeDuration(int64 ms) const {
  return pagespeed::formatters::FormatTimeDuration(ms);
}


std::string NullLocalizer::LocalizeString(const std::string& val) const {
  return val;
}

std::string NullLocalizer::LocalizeInt(int64 val) const {
  std::ostringstream ss;
  ss << val;
  return ss.str();
}

std::string NullLocalizer::LocalizeUrl(const std::string& url) const {
  return url;
}

std::string NullLocalizer::LocalizeBytes(int64 bytes) const {
  std::ostringstream ss;
  ss << bytes;
  return ss.str();
}

std::string NullLocalizer::LocalizeTimeDuration(int64 ms) const {
  std::ostringstream ss;
  ss << ms;
  return ss.str();
}

} // namespace l10n

} // namespace pagespeed
