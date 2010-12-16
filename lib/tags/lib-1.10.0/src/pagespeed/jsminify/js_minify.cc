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

#include "pagespeed/jsminify/js_minify.h"

#include <string>

#include "base/logging.h"
#include "base/string_piece.h"

namespace {

const int kEOF = -1;  // represents the end of the input

// A token can either be a character (0-255) or one of these constants:
const int kStartToken = 256;  // the start of the input
const int kNameNumberToken = 257;  // a name, keyword, or number
const int kCCCommentToken = 258;  // a conditional compilation comment
const int kRegexToken = 259;  // a regular expression literal
const int kStringToken = 260;  // a string literal

// Is this a character that can appear in identifiers?
int IsIdentifierChar(int c) {
  // Note that backslashes can appear in identifiers due to unicode escape
  // sequences (e.g. \u03c0).
  return ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
          (c >= 'A' && c <= 'Z') || c == '_' || c == '$' || c == '\\' ||
          c >= 127);
}

class StringConsumer {
 public:
  explicit StringConsumer(std::string* output) : output_(output) {}
  void push_back(char character) {
    output_->push_back(character);
  }
  void append(const base::StringPiece& str) {
    output_->append(str.data(), str.size());
  }
  std::string* output_;
};

class SizeConsumer {
 public:
  explicit SizeConsumer(std::string* ignored) : size_(0) {}
  void push_back(char character) {
    ++size_;
  }
  void append(const base::StringPiece& str) {
    size_ += str.size();
  }
  int size_;
};

template<typename OutputConsumer>
class Minifier {
 public:
  Minifier(const base::StringPiece& input, std::string* output);
  ~Minifier() {}

  // Return a pointer to an OutputConsumer instance if minification was
  // successful, NULL otherwise.
  OutputConsumer* GetOutput();

 private:
  int Peek();
  void ChangeToken(int next_token);
  void InsertSpaceIfNeeded();
  void ConsumeBlockComment();
  void ConsumeLineComment();
  void ConsumeNameOrNumber();
  void ConsumeRegex();
  void ConsumeString();
  void Minify();

  // Represents what kind of whitespace we've seen since the last token:
  //   NO_WHITESPACE means that there is no whitespace between the tokens.
  //   SPACE means there's been at least one space/tab, but no linebreaks.
  //   LINEBREAK means there's been at least one linebreak.
  enum Whitespace { NO_WHITESPACE, SPACE, LINEBREAK };

  const base::StringPiece input_;
  int index_;
  OutputConsumer output_;
  Whitespace whitespace_;  // whitespace since the previous token
  int prev_token_;
  bool error_;
};

template<typename OutputConsumer>
Minifier<OutputConsumer>::Minifier(const base::StringPiece& input,
                                   std::string* output)
  : input_(input),
    index_(0),
    output_(output),
    whitespace_(NO_WHITESPACE),
    prev_token_(kStartToken),
    error_(false) {}

// Return the next character after index_, or kEOF if there aren't any more.
template<typename OutputConsumer>
int Minifier<OutputConsumer>::Peek() {
  return (index_ + 1 < input_.size() ?
          static_cast<int>(input_[index_ + 1]) : kEOF);
}

// Switch to a new prev_token, and insert a newline if necessary.  Call this
// right before appending a token onto the output.
template<typename OutputConsumer>
void Minifier<OutputConsumer>::ChangeToken(int next_token) {
  // If there've been any linebreaks since the previous token, we may need to
  // insert a linebreak here to avoid running afoul of semicolon insertion
  // (that is, the code may be relying on semicolon insertion here, and
  // removing the linebreak would break it).
  if (whitespace_ == LINEBREAK) {
    switch (prev_token_) {
      // These are tokens immediately after which a semicolon should never be
      // inserted.
      case kStartToken:
      case '=':
      case '<':
      case '>':
      case ';':
      case ':':
      case '?':
      case '|':
      case '&':
      case '^':
      case '*':
      case '/':
      case '!':
      case ',':
      case '(':
      case '[':
      case '{':
        break;
      default:
        switch (next_token) {
          // These are tokens immediately before which a semicolon should never
          // be inserted.
          case ')':
          case ']':
          case '}':
            break;
          // Otherwise, we should insert a linebreak to be safe.
          default:
            output_.push_back('\n');
        }
    }
  }
  whitespace_ = NO_WHITESPACE;
  prev_token_ = next_token;
}

// If there's been any whitespace since the previous token, insert some
// whitespace now to separate the previous token from the next token.
template<typename OutputConsumer>
void Minifier<OutputConsumer>::InsertSpaceIfNeeded() {
  switch (whitespace_) {
    case SPACE:
      output_.push_back(' ');
      break;
    case LINEBREAK:
      output_.push_back('\n');
      break;
    default:
      break;
  }
  whitespace_ = NO_WHITESPACE;
}

template<typename OutputConsumer>
void Minifier<OutputConsumer>::ConsumeBlockComment() {
  DCHECK(index_ + 1 < input_.size());
  DCHECK(input_[index_] == '/');
  DCHECK(input_[index_ + 1] == '*');
  const int begin = index_;
  index_ += 2;
  const bool may_be_ccc = (index_ < input_.size() && input_[index_] == '@');
  while (index_ < input_.size()) {
    if (input_[index_] == '*' && Peek() == '/') {
      index_ += 2;
      if (may_be_ccc && input_[index_ - 3] == '@') {
        ChangeToken(kCCCommentToken);
        output_.append(input_.substr(begin, index_ - begin));
      } else {
        whitespace_ = SPACE;
      }
      return;
    }
    ++index_;
  }
  // If we reached EOF without the comment being closed, then this is an error.
  error_ = true;
}

template<typename OutputConsumer>
void Minifier<OutputConsumer>::ConsumeLineComment() {
  while (index_ < input_.size() && input_[index_] != '\n') {
    ++index_;
  }
  whitespace_ = LINEBREAK;
}

// Consume a keyword, name, or number.
template<typename OutputConsumer>
void Minifier<OutputConsumer>::ConsumeNameOrNumber() {
  if (prev_token_ == kNameNumberToken || prev_token_ == kRegexToken) {
    InsertSpaceIfNeeded();
  }
  ChangeToken(kNameNumberToken);
  while (index_ < input_.size() && IsIdentifierChar(input_[index_])) {
    output_.push_back(input_[index_]);
    ++index_;
  }
}

template<typename OutputConsumer>
void Minifier<OutputConsumer>::ConsumeRegex() {
  DCHECK(index_ < input_.size());
  DCHECK(input_[index_] == '/');
  const int begin = index_;
  ++index_;
  while (index_ < input_.size()) {
    const char ch = input_[index_];
    ++index_;
    if (ch == '\\') {
      // If we see a backslash, don't check the next character (this is mainly
      // relevant if the next character is a slash that would otherwise close
      // the regex literal).
      ++index_;
    } else if (ch == '/') {
      // Don't accidentally create a line comment.
      if (prev_token_ == '/') {
        InsertSpaceIfNeeded();
      }
      ChangeToken(kRegexToken);
      output_.append(input_.substr(begin, index_ - begin));
      return;
    } else if (ch == '\n') {
      break;  // error
    }
  }
  // If we reached EOF without the regex being closed, then this is an error.
  error_ = true;
}

template<typename OutputConsumer>
void Minifier<OutputConsumer>::ConsumeString() {
  DCHECK(index_ < input_.size());
  const int begin = index_;
  const char quote = input_[begin];
  DCHECK(quote == '"' || quote == '\'');
  ++index_;
  while (index_ < input_.size()) {
    const char ch = input_[index_];
    ++index_;
    if (ch == '\\') {
      ++index_;
    } else {
      if (ch == quote) {
        ChangeToken(kStringToken);
        output_.append(input_.substr(begin, index_ - begin));
        return;
      }
    }
  }
  // If we reached EOF without the string being closed, then this is an error.
  error_ = true;
}

template<typename OutputConsumer>
void Minifier<OutputConsumer>::Minify() {
  while (index_ < input_.size() && !error_) {
    const char ch = input_[index_];
    // Track whitespace since the previous token.  NO_WHITESPACE means no
    // whitespace; LINEBREAK means there's been at least one linebreak; SPACE
    // means there's been spaces/tabs, but no linebreaks.
    if (ch == '\n' || ch == '\r') {
      whitespace_ = LINEBREAK;
      ++index_;
    }
    else if (ch == ' ' || ch == '\t') {
      if (whitespace_ == NO_WHITESPACE) {
        whitespace_ = SPACE;
      }
      ++index_;
    }
    // Strings:
    else if (ch == '\'' || ch == '"') {
      ConsumeString();
    }
    // A slash could herald a line comment, a block comment, a regex literal,
    // or a mere division operator; we need to figure out which it is.
    // Differentiating between division and regexes is mostly impossible
    // without parsing, so we do our best based on the previous token.
    else if (ch == '/') {
      const int next = Peek();
      if (next == '/') {
        ConsumeLineComment();
      } else if (next == '*') {
        ConsumeBlockComment();
      }
      // If the slash is following a primary expression (like a literal, or
      // (...), or foo[0]), then it's definitely a division operator.  These
      // are previous tokens for which (I think) we can be sure that we're
      // following a primary expression.
      else if (prev_token_ == kNameNumberToken ||
               prev_token_ == kRegexToken ||
               prev_token_ == kStringToken ||
               prev_token_ == ')' ||
               prev_token_ == ']') {
        ChangeToken('/');
        output_.push_back(ch);
        ++index_;
      }
      // If we can't be sure it's division, then we must assume it's a regex so
      // that we don't remove whitespace that we shouldn't.  There are cases
      // that we'll get wrong, but it's hard to do better without parsing.
      else {
        ConsumeRegex();
      }
    }
    // Identifiers, keywords, and numeric literals:
    else if (IsIdentifierChar(ch)) {
      ConsumeNameOrNumber();
    }
    // Treat <!-- as a line comment.  Note that the substr() here is very
    // efficient because input_ is a StringPiece, not a std::string.
    else if (ch == '<' && input_.substr(index_).starts_with("<!--")) {
      ConsumeLineComment();
    }
    // Treat --> as a line comment if it's at the start of a line.
    else if (ch == '-' &&
             (whitespace_ == LINEBREAK || prev_token_ == kStartToken) &&
             input_.substr(index_).starts_with("-->")) {
      ConsumeLineComment();
    }
    // Copy other characters over verbatim, but make sure not to join two +
    // tokens into ++ or two - tokens into --, and avoid minifying the sequence
    // of tokens < ! -- into an SGML line comment.
    else {
      if ((prev_token_ == ch && (ch == '+' || ch == '-')) ||
          (prev_token_ == '<' && ch == '!') ||
          (prev_token_ == '!' && ch == '-')) {
        InsertSpaceIfNeeded();
      }
      ChangeToken(ch);
      output_.push_back(ch);
      ++index_;
    }
  }
}

template<typename OutputConsumer>
OutputConsumer* Minifier<OutputConsumer>::GetOutput() {
  Minify();
  if (!error_) {
    return &output_;
  }
  return NULL;
}

}  // namespace

namespace pagespeed {

namespace jsminify {

bool MinifyJs(const base::StringPiece& input, std::string* out) {
  Minifier<StringConsumer> minifier(input, out);
  return (minifier.GetOutput() != NULL);
}

bool GetMinifiedJsSize(const base::StringPiece& input, int* minimized_size) {
  Minifier<SizeConsumer> minifier(input, NULL);
  SizeConsumer* output = minifier.GetOutput();
  if (output) {
    *minimized_size = output->size_;
    return true;
  } else {
    return false;
  }
}

}  // namespace jsminify

}  // namespace pagespeed
