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

// Command line utility to minify HTML

#include <stdio.h>

#include <fstream>
#include <string>

#include "pagespeed/html/html_minifier.h"

int main(int argc, char** argv) {
  if (argc != 3) {
    fprintf(stderr, "Usage: minify_html <input> <output>\n");
    return 1;
  }

  std::string filename = argv[1];
  std::ifstream in(filename.c_str(), std::ios::in | std::ios::binary);
  if (!in) {
    fprintf(stderr, "Could not read input from %s\n", filename.c_str());
    return 1;
  }

  in.seekg(0, std::ios::end);
  const int length = in.tellg();
  in.seekg(0, std::ios::beg);

  std::string original;
  original.resize(length);
  in.read(&original[0], length);
  in.close();

  std::string minified;

  pagespeed::html::HtmlMinifier html_minifier;
  html_minifier.MinifyHtml(filename, original, &minified);

  std::ofstream out(argv[2], std::ios::out | std::ios::binary);
  if (!out) {
    fprintf(stderr, "Error opening %s for write\n", argv[2]);
    return 1;
  }
  out.write(minified.c_str(), minified.size());
  out.close();
  return 0;
}
