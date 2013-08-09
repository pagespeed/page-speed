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

#ifndef PAGESPEED_CORE_FORMATTER_H_
#define PAGESPEED_CORE_FORMATTER_H_

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "pagespeed/l10n/user_facing_string.h"
#include "pagespeed/proto/pagespeed_proto_formatter.pb.h"

namespace pagespeed {

class Rule;

FormatArgument BytesArgument(const std::string& key, int64 bytes);

FormatArgument DurationArgument(const std::string& key, int64 milliseconds);

FormatArgument IntArgument(const std::string& key, int64 integer);

FormatArgument PercentageArgument(const std::string& key,
                                  int64 numerator, int64 denominator);

// Used for pre-formatted strings such as code blocks or stack traces.
FormatArgument VerbatimStringArgument(const std::string& key,
                                      const std::string& str);

FormatArgument StringArgument(const std::string& key, const std::string& str);

// A URL; in contexts that allow hyperlinks, the given URL should be used both
// as the href and as the displayed label.
FormatArgument UrlArgument(const std::string& key, const std::string& url);

// Used for turning a portion of the translated text into a hyperlink.  This
// format argument uses two placeholders: if key is "FOO", the placeholders are
// "%(BEGIN_FOO)s" and "%(END_FOO)s".
FormatArgument HyperlinkArgument(const std::string& key,
                                 const std::string& href);

class UrlFormatter {
 public:
  UrlFormatter() {}
  virtual ~UrlFormatter() {}

  virtual void AddDetail(
      const UserFacingString& format_str,
      const std::vector<const FormatArgument*>& arguments) = 0;

  virtual void SetAssociatedResultId(int id) = 0;

  // Convenience methods:
  void AddDetail(const UserFacingString& format_str);
  void AddDetail(const UserFacingString& format_str,
                 const FormatArgument& arg1);
  void AddDetail(const UserFacingString& format_str,
                 const FormatArgument& arg1,
                 const FormatArgument& arg2);
  void AddDetail(const UserFacingString& format_str,
                 const FormatArgument& arg1,
                 const FormatArgument& arg2,
                 const FormatArgument& arg3);

 private:
  DISALLOW_COPY_AND_ASSIGN(UrlFormatter);
};

class UrlBlockFormatter {
 public:
  UrlBlockFormatter() {}
  virtual ~UrlBlockFormatter() {}

  // Create, add, and return a new UrlFormatter.  The returned object has the
  // same lifetime as the parent.
  virtual UrlFormatter* AddUrlResult(
      const UserFacingString& format_str,
      const std::vector<const FormatArgument*>& arguments) = 0;

  // Convenience methods:
  UrlFormatter* AddUrl(const std::string& url);
  UrlFormatter* AddUrlResult(const UserFacingString& format_str);
  UrlFormatter* AddUrlResult(const UserFacingString& format_str,
                             const FormatArgument& arg1);
  UrlFormatter* AddUrlResult(const UserFacingString& format_str,
                             const FormatArgument& arg1,
                             const FormatArgument& arg2);
  UrlFormatter* AddUrlResult(const UserFacingString& format_str,
                             const FormatArgument& arg1,
                             const FormatArgument& arg2,
                             const FormatArgument& arg3);
  UrlFormatter* AddUrlResult(const UserFacingString& format_str,
                             const FormatArgument& arg1,
                             const FormatArgument& arg2,
                             const FormatArgument& arg3,
                             const FormatArgument& arg4);
  UrlFormatter* AddUrlResult(const UserFacingString& format_str,
                             const FormatArgument& arg1,
                             const FormatArgument& arg2,
                             const FormatArgument& arg3,
                             const FormatArgument& arg4,
                             const FormatArgument& arg5);
  UrlFormatter* AddUrlResult(const UserFacingString& format_str,
                             const FormatArgument& arg1,
                             const FormatArgument& arg2,
                             const FormatArgument& arg3,
                             const FormatArgument& arg4,
                             const FormatArgument& arg5,
                             const FormatArgument& arg6);
  UrlFormatter* AddUrlResult(const UserFacingString& format_str,
                             const FormatArgument& arg1,
                             const FormatArgument& arg2,
                             const FormatArgument& arg3,
                             const FormatArgument& arg4,
                             const FormatArgument& arg5,
                             const FormatArgument& arg6,
                             const FormatArgument& arg7);

 private:
  DISALLOW_COPY_AND_ASSIGN(UrlBlockFormatter);
};

class RuleFormatter {
 public:
  RuleFormatter() {}
  virtual ~RuleFormatter() {}

  // Create, add, and return a new UrlBlockFormatter.  The returned object has
  // the same lifetime as the parent.
  virtual UrlBlockFormatter* AddUrlBlock(
      const UserFacingString& format_str,
      const std::vector<const FormatArgument*>& arguments) = 0;

  // Convenience methods:
  UrlBlockFormatter* AddUrlBlock(const UserFacingString& format_str);
  UrlBlockFormatter* AddUrlBlock(const UserFacingString& format_str,
                                 const FormatArgument& arg1);
  UrlBlockFormatter* AddUrlBlock(const UserFacingString& format_str,
                                 const FormatArgument& arg1,
                                 const FormatArgument& arg2);
  UrlBlockFormatter* AddUrlBlock(const UserFacingString& format_str,
                                 const FormatArgument& arg1,
                                 const FormatArgument& arg2,
                                 const FormatArgument& arg3);
  UrlBlockFormatter* AddUrlBlock(const UserFacingString& format_str,
                                 const FormatArgument& arg1,
                                 const FormatArgument& arg2,
                                 const FormatArgument& arg3,
                                 const FormatArgument& arg4);
  UrlBlockFormatter* AddUrlBlock(const UserFacingString& format_str,
                                 const FormatArgument& arg1,
                                 const FormatArgument& arg2,
                                 const FormatArgument& arg3,
                                 const FormatArgument& arg4,
                                 const FormatArgument& arg5);
  UrlBlockFormatter* AddUrlBlock(const UserFacingString& format_str,
                                 const FormatArgument& arg1,
                                 const FormatArgument& arg2,
                                 const FormatArgument& arg3,
                                 const FormatArgument& arg4,
                                 const FormatArgument& arg5,
                                 const FormatArgument& arg6);
  UrlBlockFormatter* AddUrlBlock(const UserFacingString& format_str,
                                 const FormatArgument& arg1,
                                 const FormatArgument& arg2,
                                 const FormatArgument& arg3,
                                 const FormatArgument& arg4,
                                 const FormatArgument& arg5,
                                 const FormatArgument& arg6,
                                 const FormatArgument& arg7);

 private:
  DISALLOW_COPY_AND_ASSIGN(RuleFormatter);
};

class Formatter {
 public:
  Formatter() {}
  virtual ~Formatter() {}

  // Create, add, and return a new RuleFormatter.  The returned object has the
  // same lifetime as the parent.
  virtual RuleFormatter* AddRule(const Rule& rule, int score,
                                 double impact) = 0;
  // Set the overall score (from 0 to 100).
  virtual void SetOverallScore(int score) = 0;

  // Finalize the formatted results.
  virtual void Finalize() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(Formatter);
};

}  // namespace pagespeed

#endif  // PAGESPEED_CORE_FORMATTER_H_
