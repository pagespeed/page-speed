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
  DCHECK(type_ == INTEGER || type_ == BYTES || type_ == DURATION ||
         type_ == PERCENTAGE);
}

Argument::Argument(Argument::ArgumentType type, const char* value)
    : type_(type), int_value_(-1),
      string_value_(value) {
  DCHECK(type_ == STRING || type_ == URL || type_ == PRE_STRING);
}

Argument::Argument(Argument::ArgumentType type, const std::string& value)
    : type_(type), int_value_(-1),
      string_value_(value) {
  DCHECK(type_ == STRING || type_ == URL || type_ == PRE_STRING);
}

int64 Argument::int_value() const {
  DCHECK(type_ == INTEGER || type_ == BYTES || type_ == DURATION ||
         type_ == PERCENTAGE);
  return int_value_;
}

const std::string& Argument::string_value() const {
  DCHECK(type_ == STRING || type_ == URL || type_ == PRE_STRING);
  return string_value_;
}

Argument::ArgumentType Argument::type() const {
  return type_;
}

void UrlFormatter::AddDetail(const UserFacingString& format_str) {
  std::vector<const Argument*> args;
  AddDetail(format_str, args);
}

void UrlFormatter::AddDetail(const UserFacingString& format_str,
                             const Argument& arg1) {
  std::vector<const Argument*> args;
  args.push_back(&arg1);
  AddDetail(format_str, args);
}

void UrlFormatter::AddDetail(const UserFacingString& format_str,
                             const Argument& arg1,
                             const Argument& arg2) {
  std::vector<const Argument*> args;
  args.push_back(&arg1);
  args.push_back(&arg2);
  AddDetail(format_str, args);
}

void UrlFormatter::AddDetail(const UserFacingString& format_str,
                             const Argument& arg1,
                             const Argument& arg2,
                             const Argument& arg3) {
  std::vector<const Argument*> args;
  args.push_back(&arg1);
  args.push_back(&arg2);
  args.push_back(&arg3);
  AddDetail(format_str, args);
}

UrlFormatter* UrlBlockFormatter::AddUrl(const std::string& url) {
  const UserFacingString format = not_localized("$1");
  const Argument arg(Argument::URL, url);
  std::vector<const Argument*> args;
  args.push_back(&arg);
  return AddUrlResult(format, args);
}

UrlFormatter* UrlBlockFormatter::AddUrlResult(
    const UserFacingString& format_str) {
  std::vector<const Argument*> args;
  return AddUrlResult(format_str, args);
}

UrlFormatter* UrlBlockFormatter::AddUrlResult(
    const UserFacingString& format_str,
    const Argument& arg1) {
  std::vector<const Argument*> args;
  args.push_back(&arg1);
  return AddUrlResult(format_str, args);
}

UrlFormatter* UrlBlockFormatter::AddUrlResult(
    const UserFacingString& format_str,
    const Argument& arg1,
    const Argument& arg2) {
  std::vector<const Argument*> args;
  args.push_back(&arg1);
  args.push_back(&arg2);
  return AddUrlResult(format_str, args);
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
  return AddUrlResult(format_str, args);
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
  return AddUrlResult(format_str, args);
}

UrlFormatter* UrlBlockFormatter::AddUrlResult(
    const UserFacingString& format_str,
    const Argument& arg1,
    const Argument& arg2,
    const Argument& arg3,
    const Argument& arg4,
    const Argument& arg5,
    const Argument& arg6,
    const Argument& arg7) {
  std::vector<const Argument*> args;
  args.push_back(&arg1);
  args.push_back(&arg2);
  args.push_back(&arg3);
  args.push_back(&arg4);
  args.push_back(&arg5);
  args.push_back(&arg6);
  args.push_back(&arg7);
  return AddUrlResult(format_str, args);
}

UrlBlockFormatter* RuleFormatter::AddUrlBlock(
    const UserFacingString& format_str) {
  std::vector<const Argument*> args;
  return AddUrlBlock(format_str, args);
}

UrlBlockFormatter* RuleFormatter::AddUrlBlock(
    const UserFacingString& format_str,
    const Argument& arg1) {
  std::vector<const Argument*> args;
  args.push_back(&arg1);
  return AddUrlBlock(format_str, args);
}

UrlBlockFormatter* RuleFormatter::AddUrlBlock(
    const UserFacingString& format_str,
    const Argument& arg1,
    const Argument& arg2) {
  std::vector<const Argument*> args;
  args.push_back(&arg1);
  args.push_back(&arg2);
  return AddUrlBlock(format_str, args);
}

}  // namespace pagespeed
