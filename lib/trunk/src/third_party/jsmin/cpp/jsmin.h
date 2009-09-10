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

#ifndef THIRD_PARTY_JSMIN_CPP_JSMIN_H_
#define THIRD_PARTY_JSMIN_CPP_JSMIN_H_

#include <string>

namespace jsmin {

/**
 * jsmin::Minifier is a C++ port of jsmin.c. Minifier uses member
 * variables instead of the static globals used by jsmin.c. Minifier
 * also uses strings to read input and write output, where jsmin.c
 * uses stdin/stdout.
 */
class Minifier {
 public:
  /**
   * Construct a new Minifier instance that minifies the specified
   * JavaScript input.
   */
  Minifier(const char *input);
  ~Minifier();

  /**
   * @return true if minification was successful, false otherwise. If
   * false, the output string is cleared, not populated.
   */
  bool GetMinifiedOutput(std::string *out);

 private:
  // The various methods from jsmin.c, ported to this class.
  int get();
  int peek();
  int next();
  void action(int d);
  void jsmin();

  // Data members from jsmin.c, ported to this class.
  int theA;
  int theB;
  int theLookahead;

  // Our custom data members, not from jsmin.c
  const char *const input_;
  int input_index_;
  std::string output_buffer_;
  bool error_;
  bool done_;
};

}  // namespace jsmin

#endif  // THIRD_PARTY_JSMIN_CPP_JSMIN_H_
