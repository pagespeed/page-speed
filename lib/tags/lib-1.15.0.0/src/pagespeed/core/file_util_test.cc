// Copyright 2009 Google Inc. All Rights Reserved.
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

#include <string>

#include "googleurl/src/gurl.h"
#include "pagespeed/core/file_util.h"
#include "testing/gtest/include/gtest/gtest.h"

using pagespeed::ChooseOutputFilename;

namespace {

TEST(FileUtilTest, ChooseOutputFilenameSimple) {
  const GURL url("http://www.example.com/foo/bar.png");
  const std::string filename = ChooseOutputFilename(url, "image/png",
                                                    "a1b2c3d4");
  ASSERT_EQ("bar_a1b2c3d4.png", filename);
}

TEST(FileUtilTest, ChooseOutputFilenameNoBasename) {
  const GURL url("http://www.example.com/foo/.png");
  const std::string filename = ChooseOutputFilename(url, "image/png",
                                                    "a1b2c3d4");
  ASSERT_EQ("_a1b2c3d4.png", filename);
}

TEST(FileUtilTest, ChooseOutputFilenameNoDot) {
  const GURL url("http://www.example.com/bar/foo");
  const std::string filename = ChooseOutputFilename(url, "image/png",
                                                    "a1b2c3d4");
  ASSERT_EQ("foo_a1b2c3d4.png", filename);
}

TEST(FileUtilTest, ChooseOutputFilenameNoSlashes) {
  const GURL url("http://www.example.com/foo.png");
  const std::string filename = ChooseOutputFilename(url, "image/png",
                                                    "a1b2c3d4");
  ASSERT_EQ("foo_a1b2c3d4.png", filename);
}

TEST(FileUtilTest, ChooseOutputFilenameMultipleDots) {
  const GURL url("http://www.example.com/baz/foo.bar.png");
  const std::string filename = ChooseOutputFilename(url, "image/png",
                                                    "a1b2c3d4");
  ASSERT_EQ("foo.bar_a1b2c3d4.png", filename);
}

TEST(FileUtilTest, ChooseOutputFilenameDotBeforeSlash) {
  const GURL url("http://www.example.com/foo.bar/baz");
  const std::string filename = ChooseOutputFilename(url, "image/png",
                                                    "a1b2c3d4");
  ASSERT_EQ("baz_a1b2c3d4.png", filename);
}

TEST(FileUtilTest, ChooseOutputFilenameIgnoreQuery) {
  const GURL url("http://www.example.com/foo/bar?t=12");
  const std::string filename = ChooseOutputFilename(url, "image/png",
                                                    "a1b2c3d4");
  ASSERT_EQ("bar_a1b2c3d4.png", filename);
}

TEST(FileUtilTest, ChooseOutputFilenameReplaceNonPrintableChars) {
  const GURL url("http://www.example.com/foo/b%E4r");
  const std::string filename = ChooseOutputFilename(url, "image/png",
                                                    "a1b2c3d4");
  ASSERT_EQ("b_E4r_a1b2c3d4.png", filename);
}

TEST(FileUtilTest, ChooseOutputFilenameNothing) {
  const GURL url("http://www.example.com/");
  const std::string filename = ChooseOutputFilename(url, "image/png",
                                                    "a1b2c3d4");
  ASSERT_EQ("_a1b2c3d4.png", filename);
}

}  // namespace
