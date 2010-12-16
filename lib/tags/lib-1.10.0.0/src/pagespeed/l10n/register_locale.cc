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

#include "pagespeed/l10n/register_locale.h"

#include <algorithm>

#include "base/logging.h"

namespace pagespeed {

namespace l10n {

namespace {

// The locale of the master table (i.e. the locale for which translation is the
// identity transformation)
const char* kNativeLocale = "en_US";

} // namespace

using std::map;
using std::string;
using std::vector;

bool RegisterLocale::frozen_ = false;
std::map<std::string, const char**>* RegisterLocale::string_table_map_ = NULL;
std::map<std::string, size_t>* RegisterLocale::master_string_map_ = NULL;

RegisterLocale::RegisterLocale(const char* locale, const char** string_table) {
  CHECK(string_table);
  CHECK(!frozen_);

  // Instantiate the locale -> string table map if it doesn't already exist
  if (!string_table_map_) {
    string_table_map_ = new std::map<std::string, const char**>();
  }

  if (!locale) {
    string_table_map_->insert(make_pair(string(kNativeLocale), string_table));

    // Build map from master string -> table index
    CHECK(!master_string_map_); // we can only register one master string table
    master_string_map_ = new std::map<std::string, size_t>();

    for (size_t i = 0; string_table[i] != NULL; ++i) {
      master_string_map_->insert(std::make_pair(string_table[i], i));
    }
  } else {
    string_table_map_->insert(make_pair(string(locale), string_table));
  }
}

RegisterLocale::~RegisterLocale() {
  if (string_table_map_) {
    delete string_table_map_;
    string_table_map_ = NULL;
  }

  if (master_string_map_) {
    delete master_string_map_;
    master_string_map_ = NULL;
  }
}

const char** RegisterLocale::GetStringTable(const std::string& locale) {
  CHECK(frozen_);

  if (!string_table_map_)
    return NULL; // no locales have been registered

  CHECK(master_string_map_); // we must have a master string table
  std::map<std::string, const char**>::const_iterator itr =
      string_table_map_->find(locale);

  if (itr == string_table_map_->end())
    return NULL;

  return itr->second;
}

void RegisterLocale::GetAllLocales(std::vector<std::string>* out) {
  CHECK(out);
  CHECK(frozen_);

  if (!string_table_map_)
    return;
  CHECK(master_string_map_);

  std::map<std::string, const char**>::const_iterator itr;
  for (itr = string_table_map_->begin();
       itr != string_table_map_->end();
       ++itr) {
    out->push_back(itr->first);
  }

  // since the order is non-deterministic, sort to make output consistent
  std::sort(out->begin(), out->end());
}

} // namespace l10n

} // namespace pagespeed
