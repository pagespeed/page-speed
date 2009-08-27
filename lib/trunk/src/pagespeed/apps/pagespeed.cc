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

// Command line utility that runs lint rules on the provided input set.

#include <fstream>

#include <stdio.h>

#include "base/logging.h"
#include "base/stl_util-inl.h"  // for STLDeleteContainerPointers
#include "base/string_util.h"
#include "google/protobuf/text_format.h"
#include "pagespeed/core/engine.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/pagespeed_input.pb.h"
#include "pagespeed/core/pagespeed_output.pb.h"
#include "pagespeed/rules/rule_provider.h"

namespace {

#if defined(_WINDOWS)
// Windows defines its own variants of the printf functions, so we
// must write our own version of snprintf that delegates to the
// Windows implementation.
int snprintf(char *str, size_t size, const char *format, ...) {
  va_list args;
  va_start(args, format);
  int result = vsnprintf_s(str, size, _TRUNCATE, format, args);
  va_end(args);
  return result;
}
#endif

template <typename FormatArguments>
std::string Format(const std::string& format_str, const FormatArguments& args) {
  std::vector<string16> subst;

  for (typename FormatArguments::const_iterator iter = args.begin(),
           end = args.end();
       iter != end;
       ++iter) {
    const pagespeed::FormatArgument& arg = *iter;
    switch (arg.type()) {
      case pagespeed::FormatArgument::URL:
        subst.push_back(UTF8ToUTF16(arg.url()));
        break;
      case pagespeed::FormatArgument::STRING_LITERAL:
        subst.push_back(UTF8ToUTF16(arg.string_literal()));
        break;
      case pagespeed::FormatArgument::INT_LITERAL:
        subst.push_back(IntToString16(arg.int_literal()));
        break;
      case pagespeed::FormatArgument::DOUBLE_LITERAL:
        char buffer[100];
        snprintf(buffer, arraysize(buffer), "%.1f", arg.double_literal());
        subst.push_back(UTF8ToUTF16(buffer));
        break;
      default:
        CHECK(false);
        break;
    }
  }

  return UTF16ToUTF8(
      ReplaceStringPlaceholders(UTF8ToUTF16(format_str), subst, NULL));
}

void Dump(const pagespeed::ResultText& result, int indent = 0) {
  const std::string& str = Format(result.format(), result.args());

  for (int indent_idx = 0; indent_idx < indent; indent_idx++) {
    printf("  ");
  }

  switch (indent) {
    case 0:
      // header
      printf("_%s_\n", str.c_str());
      break;
    case 1:
      // regular text
      printf("%s\n", str.c_str());
      break;
    default:
      // bullet
      printf("* %s\n", str.c_str());
      break;
  }

  for (int idx = 0; idx < result.children_size(); idx++) {
    Dump(result.children(idx), indent + 1);
  }
}

void ProcessInput(const pagespeed::ProtoInput& input_proto) {
  std::vector<pagespeed::Rule*> rules;
  pagespeed::rule_provider::AppendCoreRules(&rules);

  // Ownership of rules is transferred to the Engine instance.
  pagespeed::Engine engine(rules);

  pagespeed::PagespeedInput input(&input_proto);

  std::vector<pagespeed::ResultText*> results;
  engine.ComputeResultText(input, &results);

  for (std::vector<pagespeed::ResultText*>::const_iterator
           iter = results.begin(), end = results.end();
       iter != end;
       ++iter) {
    pagespeed::ResultText* result = *iter;
    Dump(*result);
    printf("\n");
  }
  STLDeleteContainerPointers(results.begin(), results.end());
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: pagespeed <input>\n");
    return 1;
  }

  std::string filename = argv[1];
  std::ifstream in(filename.c_str());
  if (!in) {
    fprintf(stderr, "Could not read input from %s\n", filename.c_str());
    return 1;
  }

  std::string file_contents;
  std::string line;
  while (std::getline(in, line)) {
    file_contents += line;
    file_contents += '\n';
  }

  pagespeed::ProtoInput input;
  bool success = ::google::protobuf::TextFormat::ParseFromString(
      file_contents, &input);
  CHECK(success);

  ProcessInput(input);

  return 0;
}
