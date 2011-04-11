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

#include "pagespeed/core/formatter.h"

#include "base/logging.h"
#include "pagespeed/core/rule.h"
#include "pagespeed/l10n/l10n.h"  // for not_localized()

namespace {

const std::string kEmptyString;
const pagespeed::UserFacingString kLocalizableEmptyString;
const std::vector<const pagespeed::Argument*> kEmptyParameterList;

}  // namespace

namespace pagespeed {

Argument::Argument(Argument::ArgumentType type, int64 value)
    : type_(type),
      int_value_(value) {
  DCHECK(type_ == INTEGER || type_ == BYTES || type_ == DURATION);
}

Argument::Argument(Argument::ArgumentType type, const char* value)
    : type_(type), int_value_(-1),
      string_value_(value) {
  DCHECK(type_ == STRING || type_ == URL);
}

Argument::Argument(Argument::ArgumentType type, const std::string& value)
    : type_(type), int_value_(-1),
      string_value_(value) {
  DCHECK(type_ == STRING || type_ == URL);
}

int64 Argument::int_value() const {
  DCHECK(type_ == INTEGER || type_ == BYTES || type_ == DURATION);
  return int_value_;
}

const std::string& Argument::string_value() const {
  DCHECK(type_ == STRING || type_ == URL);
  return string_value_;
}

Argument::ArgumentType Argument::type() const {
  return type_;
}

FormatterParameters::FormatterParameters(const UserFacingString* format_str)
    : format_str_(format_str),
      arguments_(&kEmptyParameterList) {
  DCHECK_NE(format_str, static_cast<const UserFacingString*>(NULL));
}

FormatterParameters::FormatterParameters(
    const UserFacingString* format_str,
    const std::vector<const Argument*>* arguments)
    : format_str_(format_str),
      arguments_(arguments) {
  DCHECK_NE(format_str, static_cast<const UserFacingString*>(NULL));
  DCHECK_NE(arguments,
            static_cast<std::vector<const pagespeed::Argument*>*>(NULL));
}

const UserFacingString& FormatterParameters::format_str() const {
  if (format_str_ != NULL) {
    return *format_str_;
  } else {
    return kLocalizableEmptyString;
  }
}

const std::vector<const Argument*>& FormatterParameters::arguments() const {
  if (arguments_ != NULL) {
    return *arguments_;
  } else {
    return kEmptyParameterList;
  }
}

void UrlFormatter::AddDetail(const UserFacingString& format_str) {
  std::vector<const Argument*> args;
  const FormatterParameters params(&format_str, &args);
  AddDetail(params);
}

void UrlFormatter::AddDetail(const UserFacingString& format_str,
                             const Argument& arg1) {
  std::vector<const Argument*> args;
  args.push_back(&arg1);
  const FormatterParameters params(&format_str, &args);
  AddDetail(params);
}

void UrlFormatter::AddDetail(const UserFacingString& format_str,
                             const Argument& arg1,
                             const Argument& arg2) {
  std::vector<const Argument*> args;
  args.push_back(&arg1);
  args.push_back(&arg2);
  const FormatterParameters params(&format_str, &args);
  AddDetail(params);
}

UrlFormatter* UrlBlockFormatter::AddUrl(const std::string& url) {
  const UserFacingString format = not_localized("$1");
  const Argument arg(Argument::URL, url);
  std::vector<const Argument*> args;
  args.push_back(&arg);
  const FormatterParameters params(&format, &args);
  return AddUrlResult(params);
}

UrlFormatter* UrlBlockFormatter::AddUrlResult(
    const UserFacingString& format_str) {
  std::vector<const Argument*> args;
  const FormatterParameters params(&format_str, &args);
  return AddUrlResult(params);
}

UrlFormatter* UrlBlockFormatter::AddUrlResult(
    const UserFacingString& format_str,
    const Argument& arg1) {
  std::vector<const Argument*> args;
  args.push_back(&arg1);
  const FormatterParameters params(&format_str, &args);
  return AddUrlResult(params);
}

UrlFormatter* UrlBlockFormatter::AddUrlResult(
    const UserFacingString& format_str,
    const Argument& arg1,
    const Argument& arg2) {
  std::vector<const Argument*> args;
  args.push_back(&arg1);
  args.push_back(&arg2);
  const FormatterParameters params(&format_str, &args);
  return AddUrlResult(params);
}

UrlFormatter* UrlBlockFormatter::AddUrlResult(
    const UserFacingString& format_str,
    const Argument& arg1,
    const Argument& arg2,
    const Argument& arg3) {
  std::vector<const Argument*> args;
  args.push_back(&arg1);
  args.push_back(&arg2);
  args.push_back(&arg3);
  const FormatterParameters params(&format_str, &args);
  return AddUrlResult(params);
}

UrlFormatter* UrlBlockFormatter::AddUrlResult(
    const UserFacingString& format_str,
    const Argument& arg1,
    const Argument& arg2,
    const Argument& arg3,
    const Argument& arg4) {
  std::vector<const Argument*> args;
  args.push_back(&arg1);
  args.push_back(&arg2);
  args.push_back(&arg3);
  args.push_back(&arg4);
  const FormatterParameters params(&format_str, &args);
  return AddUrlResult(params);
}

UrlBlockFormatter* RuleFormatter::AddUrlBlock(
    const UserFacingString& format_str) {
  std::vector<const Argument*> args;
  const FormatterParameters params(&format_str, &args);
  return AddUrlBlock(params);
}

UrlBlockFormatter* RuleFormatter::AddUrlBlock(
    const UserFacingString& format_str,
    const Argument& arg1) {
  std::vector<const Argument*> args;
  args.push_back(&arg1);
  const FormatterParameters params(&format_str, &args);
  return AddUrlBlock(params);
}

UrlBlockFormatter* RuleFormatter::AddUrlBlock(
    const UserFacingString& format_str,
    const Argument& arg1,
    const Argument& arg2) {
  std::vector<const Argument*> args;
  args.push_back(&arg1);
  args.push_back(&arg2);
  const FormatterParameters params(&format_str, &args);
  return AddUrlBlock(params);
}

}  // namespace pagespeed
