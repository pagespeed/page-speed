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

#include "base/memory/scoped_ptr.h"
#include "pagespeed/l10n/l10n.h"
#include "pagespeed/l10n/register_locale.h"
#include "pagespeed/testing/pagespeed_test.h"

using pagespeed::l10n::Localizer;
using pagespeed::l10n::GettextLocalizer;
using std::vector;
using std::string;

namespace pagespeed {

namespace l10n {

class GettextLocalizerTest : public ::testing::Test {
 protected:
  void TestLocaleString(const std::string& locale,
                        const std::string& language,
                        const std::string& country,
                        const std::string& encoding) {
    std::string language_out, country_out, encoding_out;

    ParseLocaleString(locale, &language_out, &country_out, &encoding_out);
    EXPECT_EQ(language, language_out);
    EXPECT_EQ(country, country_out);
    EXPECT_EQ(encoding, encoding_out);
  }
};

TEST_F(GettextLocalizerTest, CreateTest) {
  scoped_ptr<GettextLocalizer> loc;

  vector<string> locales;
  pagespeed::l10n::RegisterLocale::GetAllLocales(&locales);
  ASSERT_EQ(static_cast<size_t>(4), locales.size());
  ASSERT_EQ("en_US", locales[0]);
  ASSERT_EQ("test", locales[1]);
  ASSERT_EQ("test_empty", locales[2]);
  ASSERT_EQ("test_encoding", locales[3]);

  loc.reset(GettextLocalizer::Create("test"));
  ASSERT_TRUE(loc.get() != NULL);
  ASSERT_STREQ("test", loc->GetLocale());

  ASSERT_EQ(NULL, GettextLocalizer::Create("test2_bad"));
}

TEST_F(GettextLocalizerTest, LocaleNameTest) {
  scoped_ptr<GettextLocalizer> loc;

  // Test case insensitivity.
  loc.reset(GettextLocalizer::Create("tEsT_eMpTY"));
  ASSERT_TRUE(loc.get() != NULL);
  ASSERT_STREQ("tEsT_eMpTY", loc->GetLocale());

  // Test locale fallback.
  loc.reset(GettextLocalizer::Create("test_unknown"));
  ASSERT_TRUE(loc.get() != NULL);
  ASSERT_STREQ("test", loc->GetLocale());

  loc.reset(GettextLocalizer::Create("test"));
  ASSERT_TRUE(loc.get() != NULL);
  ASSERT_STREQ("test", loc->GetLocale());
}

TEST_F(GettextLocalizerTest, StringTest) {
  scoped_ptr<GettextLocalizer> loc(GettextLocalizer::Create("test"));
  ASSERT_TRUE(loc.get() != NULL);

  std::string out;
  ASSERT_TRUE(loc->LocalizeString(std::string("Avoid CSS @import"), &out));
  ASSERT_EQ("@IMPORT css aVOID", out);

  ASSERT_FALSE(loc->LocalizeString("test string", &out));
  ASSERT_EQ("test string", out);
}

TEST_F(GettextLocalizerTest, OtherTest) {
  scoped_ptr<GettextLocalizer> loc(GettextLocalizer::Create("test"));
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
TEST_F(GettextLocalizerTest, EncodingTest) {
  scoped_ptr<GettextLocalizer> loc(GettextLocalizer::Create("test_encoding"));
  ASSERT_TRUE(loc.get() != NULL);
  ASSERT_STREQ("test_encoding", loc->GetLocale());

  const char* original = "Avoid CSS @import";
  const char* encoded = "\xed\x94\xbc\xed\x95\x98 @ "
      "\xec\x88\x98\xec\x9e\x85\xec\x9d\x84 CSS\xeb\xa5\xbc";

  std::string out;
  ASSERT_TRUE(loc->LocalizeString(std::string(original), &out));
  ASSERT_EQ(encoded, out);

  // Test requesting encodings.
  loc.reset(GettextLocalizer::Create("test_encoding.utf-8"));
  ASSERT_TRUE(loc.get() != NULL);

  loc.reset(GettextLocalizer::Create("test_encoding.UTF-8"));
  ASSERT_TRUE(loc.get() != NULL);

  ASSERT_TRUE(NULL == GettextLocalizer::Create("test_encoding.UTF8"));
  ASSERT_TRUE(NULL == GettextLocalizer::Create("test_encoding.utf-32"));
}

TEST_F(GettextLocalizerTest, ErrorTests) {
  scoped_ptr<GettextLocalizer> loc(GettextLocalizer::Create("test_empty"));
  ASSERT_TRUE(loc.get() != NULL);
  ASSERT_STREQ("test_empty", loc->GetLocale());

  std::string out;
  ASSERT_FALSE(loc->LocalizeString("no translation", &out));
  ASSERT_EQ("no translation", out);

  ASSERT_FALSE(loc->LocalizeBytes(53, &out));
  ASSERT_EQ("53B", out);
}

TEST_F(GettextLocalizerTest, ParseLocaleStringTest) {
  TestLocaleString("", "", "", "");
  TestLocaleString("en_US.utf-8", "en", "US", "utf-8");
  TestLocaleString("en_US", "en", "US", "");
  TestLocaleString("en", "en", "", "");
  TestLocaleString("en.utf-8", "en", "", "utf-8");
  TestLocaleString("_US", "", "US", "");
  TestLocaleString("_US.utf-8", "", "US", "utf-8");
  TestLocaleString(".utf-8", "", "", "utf-8");
  TestLocaleString("_.utf-8", "", "", "utf-8");
  TestLocaleString("en_US_US.utf-8", "en", "US_US", "utf-8");
  TestLocaleString("en_US_US.utf-8.utf-8", "en", "US_US", "utf-8.utf-8");

  // Test dashes instead of underscores.
  TestLocaleString("en-US.utf-8", "en", "US", "utf-8");
  TestLocaleString("en_US-US.utf-8", "en", "US-US", "utf-8");
  TestLocaleString("en-US_US.utf-8", "en", "US_US", "utf-8");

  // Test @modifiers.
  TestLocaleString("en_US.utf-8@silly", "en", "US", "utf-8");
  TestLocaleString("en_US@silly", "en", "US", "");
  TestLocaleString("en@silly", "en", "", "");
  TestLocaleString("en.utf-8@silly", "en", "", "utf-8");

  // Test NULL handling.
  ParseLocaleString("en_US.utf-8", NULL, NULL, NULL);
}

} // namespace l10n

} // namespace pagespeed
