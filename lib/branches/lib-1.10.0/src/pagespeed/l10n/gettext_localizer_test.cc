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

#include <vector>
#include <string>

#include "base/scoped_ptr.h"
#include "pagespeed/l10n/l10n.h"
#include "pagespeed/l10n/register_locale.h"
#include "pagespeed/testing/pagespeed_test.h"

using pagespeed::l10n::Localizer;
using pagespeed::l10n::GettextLocalizer;
using std::vector;
using std::string;

namespace {

TEST(GettextLocalizerTest, CreateTest) {
  scoped_ptr<GettextLocalizer> loc;

  vector<string> locales;
  pagespeed::l10n::RegisterLocale::GetAllLocales(&locales);
  ASSERT_EQ(static_cast<size_t>(4), locales.size());
  ASSERT_EQ("en_US", locales[0]);
  ASSERT_EQ("test_backwards", locales[1]);
  ASSERT_EQ("test_empty", locales[2]);
  ASSERT_EQ("test_encoding", locales[3]);

  loc.reset(GettextLocalizer::Create("test_backwards"));
  ASSERT_TRUE(loc.get() != NULL);
  ASSERT_STREQ("test_backwards", loc->GetLocale());

  ASSERT_EQ(NULL, GettextLocalizer::Create("bad_locale"));
}

TEST(GettextLocalizerTest, StringTest) {
  scoped_ptr<GettextLocalizer> loc(GettextLocalizer::Create("test_backwards"));
  ASSERT_TRUE(loc.get() != NULL);

  std::string out;
  ASSERT_TRUE(loc->LocalizeString(std::string("Avoid CSS @import"), &out));
  ASSERT_EQ("@IMPORT css aVOID", out);

  ASSERT_FALSE(loc->LocalizeString("test string", &out));
  ASSERT_EQ("test string", out);
}

TEST(GettextLocalizerTest, OtherTest) {
  scoped_ptr<GettextLocalizer> loc(GettextLocalizer::Create("test_backwards"));
  ASSERT_TRUE(loc.get() != NULL);

  std::string out;
  ASSERT_TRUE(loc->LocalizeInt(1234, &out));
  ASSERT_EQ("1234", out);

  ASSERT_TRUE(loc->LocalizeUrl("http://www.google.com", &out));
  ASSERT_EQ("http://www.google.com", out);

  ASSERT_TRUE(loc->LocalizeBytes(53, &out));
  ASSERT_EQ("53b", out);

  ASSERT_TRUE(loc->LocalizeBytes(5430, &out));
  ASSERT_EQ("5.3kIb", out);

  ASSERT_TRUE(loc->LocalizeBytes(53535353, &out));
  ASSERT_EQ("51.1mIb", out);

  ASSERT_FALSE(loc->LocalizeTimeDuration(6000, &out));
  ASSERT_EQ("6 seconds", out);
}

// Tests that utf8-encoding translations make it through the entire pipeline
TEST(GettextLocalizerTest, EncodingTest) {
  scoped_ptr<GettextLocalizer> loc(GettextLocalizer::Create("test_encoding"));
  ASSERT_TRUE(loc.get() != NULL);
  ASSERT_STREQ("test_encoding", loc->GetLocale());

  const char* original = "Avoid CSS @import";
  const char* encoded = "\xed\x94\xbc\xed\x95\x98 @ "
      "\xec\x88\x98\xec\x9e\x85\xec\x9d\x84 CSS\xeb\xa5\xbc";

  std::string out;
  ASSERT_TRUE(loc->LocalizeString(std::string(original), &out));
  ASSERT_EQ(encoded, out);
}

TEST(GettextLocalizerTest, ErrorTests) {
  scoped_ptr<GettextLocalizer> loc(GettextLocalizer::Create("test_empty"));
  ASSERT_TRUE(loc.get() != NULL);
  ASSERT_STREQ("test_empty", loc->GetLocale());

  std::string out;
  ASSERT_FALSE(loc->LocalizeString("no translation", &out));
  ASSERT_EQ("no translation", out);

  ASSERT_FALSE(loc->LocalizeBytes(53, &out));
  ASSERT_EQ("53B", out);
}

} // namespace
