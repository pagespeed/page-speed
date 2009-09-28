/*
 * This file contains a C++ port of jsmin.c. The copyright notice
 * below is the copyright notice from jsmin.c.
 */

/* jsmin.c
   2008-08-03

Copyright (c) 2002 Douglas Crockford  (www.crockford.com)

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

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
  explicit Minifier(const char *input);
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
