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
#include "pagespeed/proto/pagespeed_proto_formatter.pb.h"

namespace pagespeed {

FormatArgument BytesArgument(int64 bytes) {
  FormatArgument argument;
  argument.set_type(FormatArgument::BYTES);
  argument.set_int_value(bytes);
  return argument;
}

FormatArgument DurationArgument(int64 milliseconds) {
  FormatArgument argument;
  argument.set_type(FormatArgument::DURATION);
  argument.set_int_value(milliseconds);
  return argument;
}

FormatArgument IntArgument(int64 value) {
  FormatArgument argument;
  argument.set_type(FormatArgument::INT_LITERAL);
  argument.set_int_value(value);
  return argument;
}

FormatArgument PercentageArgument(int64 numerator, int64 denominator) {
  FormatArgument argument;
  argument.set_type(FormatArgument::PERCENTAGE);
  // Calculate 100 * numerator / denominator using integer division.  We want
  // to round towards 50%, so that 99.5% rounds to 99% but 0.5% rounds to 1%.
  // So, if numerator is more than half of denominator, do floor division,
  // otherwise do ceiling division.  Finally, if the denominator is zero, just
  // call it 0%.
  argument.set_int_value(denominator == 0 ? 0 :
                         numerator * 2 >= denominator ?
                         (100 * numerator) / denominator :
                         (100 * numerator + denominator - 1) / denominator);
  return argument;
}

FormatArgument VerbatimStringArgument(const std::string& value) {
  FormatArgument argument;
  argument.set_type(FormatArgument::VERBATIM_STRING);
  argument.set_string_value(value);
  return argument;
}

FormatArgument StringArgument(const std::string& value) {
  FormatArgument argument;
  argument.set_type(FormatArgument::STRING_LITERAL);
  argument.set_string_value(value);
  return argument;
}

FormatArgument UrlArgument(const std::string& value) {
  FormatArgument argument;
  argument.set_type(FormatArgument::URL);
  argument.set_string_value(value);
  return argument;
}

void UrlFormatter::AddDetail(const UserFacingString& format_str) {
  std::vector<const FormatArgument*> args;
  AddDetail(format_str, args);
}

void UrlFormatter::AddDetail(const UserFacingString& format_str,
                             const FormatArgument& arg1) {
  std::vector<const FormatArgument*> args;
  args.push_back(&arg1);
  AddDetail(format_str, args);
}

void UrlFormatter::AddDetail(const UserFacingString& format_str,
                             const FormatArgument& arg1,
                             const FormatArgument& arg2) {
  std::vector<const FormatArgument*> args;
  args.push_back(&arg1);
  args.push_back(&arg2);
  AddDetail(format_str, args);
}

void UrlFormatter::AddDetail(const UserFacingString& format_str,
                             const FormatArgument& arg1,
                             const FormatArgument& arg2,
                             const FormatArgument& arg3) {
  std::vector<const FormatArgument*> args;
  args.push_back(&arg1);
  args.push_back(&arg2);
  args.push_back(&arg3);
  AddDetail(format_str, args);
}

UrlFormatter* UrlBlockFormatter::AddUrl(const std::string& url) {
  return AddUrlResult(not_localized("$1"), UrlArgument(url));
}

UrlFormatter* UrlBlockFormatter::AddUrlResult(
    const UserFacingString& format_str) {
  std::vector<const FormatArgument*> args;
  return AddUrlResult(format_str, args);
}

UrlFormatter* UrlBlockFormatter::AddUrlResult(
    const UserFacingString& format_str,
    const FormatArgument& arg1) {
  std::vector<const FormatArgument*> args;
  args.push_back(&arg1);
  return AddUrlResult(format_str, args);
}

UrlFormatter* UrlBlockFormatter::AddUrlResult(
    const UserFacingString& format_str,
    const FormatArgument& arg1,
    const FormatArgument& arg2) {
  std::vector<const FormatArgument*> args;
  args.push_back(&arg1);
  args.push_back(&arg2);
  return AddUrlResult(format_str, args);
}

UrlFormatter* UrlBlockFormatter::AddUrlResult(
    const UserFacingString& format_str,
    const FormatArgument& arg1,
    const FormatArgument& arg2,
    const FormatArgument& arg3) {
  std::vector<const FormatArgument*> args;
  args.push_back(&arg1);
  args.push_back(&arg2);
  args.push_back(&arg3);
  return AddUrlResult(format_str, args);
}

UrlFormatter* UrlBlockFormatter::AddUrlResult(
    const UserFacingString& format_str,
    const FormatArgument& arg1,
    const FormatArgument& arg2,
    const FormatArgument& arg3,
    const FormatArgument& arg4) {
  std::vector<const FormatArgument*> args;
  args.push_back(&arg1);
  args.push_back(&arg2);
  args.push_back(&arg3);
  args.push_back(&arg4);
  return AddUrlResult(format_str, args);
}

UrlFormatter* UrlBlockFormatter::AddUrlResult(
    const UserFacingString& format_str,
    const FormatArgument& arg1,
    const FormatArgument& arg2,
    const FormatArgument& arg3,
    const FormatArgument& arg4,
    const FormatArgument& arg5,
    const FormatArgument& arg6,
    const FormatArgument& arg7) {
  std::vector<const FormatArgument*> args;
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
  std::vector<const FormatArgument*> args;
  return AddUrlBlock(format_str, args);
}

UrlBlockFormatter* RuleFormatter::AddUrlBlock(
    const UserFacingString& format_str,
    const FormatArgument& arg1) {
  std::vector<const FormatArgument*> args;
  args.push_back(&arg1);
  return AddUrlBlock(format_str, args);
}

UrlBlockFormatter* RuleFormatter::AddUrlBlock(
    const UserFacingString& format_str,
    const FormatArgument& arg1,
    const FormatArgument& arg2) {
  std::vector<const FormatArgument*> args;
  args.push_back(&arg1);
  args.push_back(&arg2);
  return AddUrlBlock(format_str, args);
}

UrlBlockFormatter* RuleFormatter::AddUrlBlock(
    const UserFacingString& format_str,
    const FormatArgument& arg1,
    const FormatArgument& arg2,
    const FormatArgument& arg3,
    const FormatArgument& arg4,
    const FormatArgument& arg5,
    const FormatArgument& arg6,
    const FormatArgument& arg7) {
  std::vector<const FormatArgument*> args;
  args.push_back(&arg1);
  args.push_back(&arg2);
  args.push_back(&arg3);
  args.push_back(&arg4);
  args.push_back(&arg5);
  args.push_back(&arg6);
  args.push_back(&arg7);
  return AddUrlBlock(format_str, args);
}

}  // namespace pagespeed
