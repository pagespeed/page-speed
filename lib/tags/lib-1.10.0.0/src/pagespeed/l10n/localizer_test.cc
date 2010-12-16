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

#include "pagespeed/testing/pagespeed_test.h"

using pagespeed::l10n::Localizer;
using pagespeed::l10n::NullLocalizer;
using pagespeed::l10n::BasicLocalizer;

namespace {

TEST(LocalizerTest, BasicLocalizerTest) {
  BasicLocalizer l;
  std::string out;

  ASSERT_STREQ("en_US", l.GetLocale());
  ASSERT_TRUE(l.LocalizeString("test string", &out));
  ASSERT_EQ("test string", out);
  ASSERT_TRUE(l.LocalizeInt(3000, &out));
  ASSERT_EQ("3000", out);
  ASSERT_TRUE(l.LocalizeUrl("http://www.google.com", &out));
  ASSERT_EQ("http://www.google.com", out);
  ASSERT_TRUE(l.LocalizeBytes(3174, &out));
  ASSERT_EQ("3.1KiB", out);
  ASSERT_TRUE(l.LocalizeTimeDuration(302000, &out));
  ASSERT_EQ("5 minutes 2 seconds", out);
}

TEST(LocalizerTest, NullLocalizerTest) {
  NullLocalizer l;
  std::string out;

  ASSERT_STREQ("en_US", l.GetLocale());
  ASSERT_TRUE(l.LocalizeString("test string", &out));
  ASSERT_EQ("test string", out);
  ASSERT_TRUE(l.LocalizeInt(3000, &out));
  ASSERT_EQ("3000", out);
  ASSERT_TRUE(l.LocalizeUrl("http://www.google.com", &out));
  ASSERT_EQ("http://www.google.com", out);
  ASSERT_TRUE(l.LocalizeBytes(3174, &out));
  ASSERT_EQ("3174", out);
  ASSERT_TRUE(l.LocalizeTimeDuration(302000, &out));
  ASSERT_EQ("302000", out);
}

} // namespace
