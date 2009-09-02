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

#include "formatter.h"

#include "base/logging.h"

namespace pagespeed {

Argument::Argument(Argument::ArgumentType type, int value)
    : type_(type),
      int_value_(value) {
  CHECK(type_ == INTEGER || type_ == BYTES);
}

Argument::Argument(Argument::ArgumentType type, const std::string& value)
    : type_(type), int_value_(-1),
      string_value_(value) {
  CHECK(type_ == STRING || type_ == URL);
}

int Argument::int_value() const {
  CHECK(type_ == INTEGER || type_ == BYTES);
  return int_value_;
}

const std::string& Argument::string_value() const {
  CHECK(type_ == STRING || type_ == URL);
  return string_value_;
}

Argument::ArgumentType Argument::type() const {
  return type_;
}

Formatter::Formatter() {
}

Formatter::~Formatter() {
}

Formatter* Formatter::AddChild(const std::string& format_str) {
  return AddChild(format_str, std::vector<const Argument*>());
}

Formatter* Formatter::AddChild(const std::string& format_str,
                               const Argument& arg1) {
  std::vector<const Argument*> args;
  args.push_back(&arg1);
  return AddChild(format_str, args);
}

Formatter* Formatter::AddChild(const std::string& format_str,
                               const Argument& arg1,
                               const Argument& arg2) {
  std::vector<const Argument*> args;
  args.push_back(&arg1);
  args.push_back(&arg2);
  return AddChild(format_str, args);
}

Formatter* Formatter::AddChild(const std::string& format_str,
                               const Argument& arg1,
                               const Argument& arg2,
                               const Argument& arg3) {
  std::vector<const Argument*> args;
  args.push_back(&arg1);
  args.push_back(&arg2);
  args.push_back(&arg3);
  return AddChild(format_str, args);
}

Formatter* Formatter::AddChild(const std::string& format_str,
                               const Argument& arg1,
                               const Argument& arg2,
                               const Argument& arg3,
                               const Argument& arg4) {
  std::vector<const Argument*> args;
  args.push_back(&arg1);
  args.push_back(&arg2);
  args.push_back(&arg3);
  args.push_back(&arg4);
  return AddChild(format_str, args);
}

Formatter* Formatter::AddChild(std::string format_str,
                               const std::vector<const Argument*>& arguments) {
  active_child_.reset(NewChild(format_str, arguments));
  return active_child_.get();
}

}  // namespace pagespeed
