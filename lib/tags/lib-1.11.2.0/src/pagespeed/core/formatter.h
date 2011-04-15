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
#include "base/scoped_ptr.h"
#include "pagespeed/l10n/user_facing_string.h"

namespace pagespeed {

class Rule;

/**
 * Typed format argument representation.
 */
class Argument {
 public:
  enum ArgumentType {
    BYTES,
    INTEGER,
    STRING,
    URL,
    DURATION,
  };

  Argument(ArgumentType type, int64 value);
  Argument(ArgumentType type, const char* value);
  Argument(ArgumentType type, const std::string& value);

  int64 int_value() const;
  const std::string& string_value() const;
  ArgumentType type() const;

 private:
  ArgumentType type_;
  int64 int_value_;
  std::string string_value_;

  DISALLOW_COPY_AND_ASSIGN(Argument);
};

/**
 * Formatter format string, arguments and additional information wrapper.
 * Additional information should be interpreted directly or ignored by
 * specific formatter subclasses.
 * Note: This class does not own the pointers it refers to.
 */
class FormatterParameters {
 public:
  explicit FormatterParameters(const UserFacingString* format_str);
  FormatterParameters(const UserFacingString* format_str,
                      const std::vector<const Argument*>* arguments);

  const UserFacingString& format_str() const;
  const std::vector<const Argument*>& arguments() const;

 private:
  const UserFacingString* format_str_;
  const std::vector<const Argument*>* arguments_;

  DISALLOW_COPY_AND_ASSIGN(FormatterParameters);
};

class UrlFormatter {
 public:
  UrlFormatter() {}
  virtual ~UrlFormatter() {}

  virtual void AddDetail(const FormatterParameters& params) = 0;

  virtual void SetAssociatedResultId(int id) = 0;

  // Convenience methods:
  void AddDetail(const UserFacingString& format_str);
  void AddDetail(const UserFacingString& format_str,
                 const Argument& arg1);
  void AddDetail(const UserFacingString& format_str,
                 const Argument& arg1,
                 const Argument& arg2);

 private:
  DISALLOW_COPY_AND_ASSIGN(UrlFormatter);
};

class UrlBlockFormatter {
 public:
  UrlBlockFormatter() {}
  virtual ~UrlBlockFormatter() {}

  // Create, add, and return a new UrlFormatter.  The returned object has the
  // same lifetime as the parent.
  virtual UrlFormatter* AddUrlResult(const FormatterParameters& params) = 0;

  // Convenience methods:
  UrlFormatter* AddUrl(const std::string& url);
  UrlFormatter* AddUrlResult(const UserFacingString& format_str);
  UrlFormatter* AddUrlResult(const UserFacingString& format_str,
                             const Argument& arg1);
  UrlFormatter* AddUrlResult(const UserFacingString& format_str,
                             const Argument& arg1,
                             const Argument& arg2);
  UrlFormatter* AddUrlResult(const UserFacingString& format_str,
                             const Argument& arg1,
                             const Argument& arg2,
                             const Argument& arg3);
  UrlFormatter* AddUrlResult(const UserFacingString& format_str,
                             const Argument& arg1,
                             const Argument& arg2,
                             const Argument& arg3,
                             const Argument& arg4);

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
      const FormatterParameters& params) = 0;

  // Convenience methods:
  UrlBlockFormatter* AddUrlBlock(const UserFacingString& format_str);
  UrlBlockFormatter* AddUrlBlock(const UserFacingString& format_str,
                                 const Argument& arg1);
  UrlBlockFormatter* AddUrlBlock(const UserFacingString& format_str,
                                 const Argument& arg1,
                                 const Argument& arg2);

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
