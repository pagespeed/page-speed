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

#include <stdio.h>

#include <fstream>

#include "base/logging.h"
#include "base/string_util.h"
#include "google/protobuf/text_format.h"
#include "pagespeed/core/engine.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/formatters/text_formatter.h"
#include "pagespeed/proto/pagespeed_input.pb.h"
#include "pagespeed/proto/proto_resource_utils.h"
#include "pagespeed/rules/rule_provider.h"

namespace {

void ProcessInput(const pagespeed::ProtoInput& input_proto) {
  std::vector<pagespeed::Rule*> rules;
  pagespeed::rule_provider::AppendCoreRules(&rules);

  // Ownership of rules is transferred to the Engine instance.
  pagespeed::Engine engine(rules);
  engine.Init();

  pagespeed::PagespeedInput input;
  pagespeed::proto::PopulatePagespeedInput(input_proto, &input);

  pagespeed::formatters::TextFormatter formatter(&std::cout);
  engine.ComputeAndFormatResults(input, &formatter);
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
