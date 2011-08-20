/**
 * Copyright 2009 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "pagespeed/css/cssmin.h"

#include "base/basictypes.h"
#include "base/logging.h"
#include "base/string_piece.h"
#include "base/string_util.h"

namespace {

const int kEOF = -1;  // represents the end of the input

// A token can either be a character (0-255) or one of these constants:
const int kStartToken = 256;  // the start of the input
const int kCommentToken = 257;  // a comment (that we chose to preserve)
const int kStringToken = 259;  // a string literal

// Lovingly copied from the js minifier rule implementation.
// TODO Extract these classes to a separate file to avoid the love from
//      spreading further.
class StringConsumer {
 public:
  explicit StringConsumer(std::string* output) : output_(output) {}

  void push_back(char c) {
    output_->push_back(c);
  }

  void append(const base::StringPiece& str) {
    output_->append(str.data(), str.size());
  }

 private:
  std::string* output_;

  DISALLOW_COPY_AND_ASSIGN(StringConsumer);
};

class SizeConsumer {
 public:
  explicit SizeConsumer(std::string* ignored) : size_(0) {}

  int size() const { return size_; }

  void push_back(char c) {
    ++size_;
  }

  void append(const base::StringPiece& str) {
    size_ += str.size();
  }

 private:
  int size_;

  DISALLOW_COPY_AND_ASSIGN(SizeConsumer);
};

// Return true for any character that never needs to be separated from other
// characters via whitespace.
bool Unextendable(int c) {
  switch (c) {
    case kStartToken:
    case kCommentToken:
    case '{':
    case '}':
    case '/':
    case ';':
    case ':':
      return true;
    default:
      return false;
  }
}

// Return true for any character that must separated from other "extendable"
// characters by whitespace on the _right_ in order keep tokens separate.
bool IsExtendableOnRight(int c) {
  switch (c) {
    // N.B. Left paren/bracket, but not right -- see
    //      http://code.google.com/p/page-speed/issues/detail?id=339 and
    //      http://code.google.com/p/page-speed/issues/detail?id=265
    case '(':
    case '[':
      return false;
    default:
      return !Unextendable(c);
  }
}

// Return true for any character that must separated from other "extendable"
// characters by whitespace on the _left_ in order keep tokens separate.
bool IsExtendableOnLeft(int c) {
  switch (c) {
    case ')':
    case ']':
      return false;
    default:
      return !Unextendable(c);
  }
}

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
  void ConsumeComment();
  void ConsumeString();
  void Minify();

  // Represents what kind of whitespace we've seen since the last token:
  //   NO_WHITESPACE means that there is no whitespace between the tokens.
  //   SPACE means there's been at least one space/tab, but no linebreaks.
  //   LINEBREAK means there's been at least one linebreak.
  enum Whitespace { NO_WHITESPACE, SPACE, LINEBREAK };

  const base::StringPiece input_;
  size_t index_;
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

template<typename OutputConsumer>
OutputConsumer* Minifier<OutputConsumer>::GetOutput() {
  Minify();
  if (!error_) {
    return &output_;
  }
  return NULL;
}

// Return the next character after index_, or kEOF if there aren't any more.
template<typename OutputConsumer>
int Minifier<OutputConsumer>::Peek() {
  return (index_ + 1 < input_.size() ?
          static_cast<int>(input_[index_ + 1]) : kEOF);
}

// Switch to a new prev_token, and insert whitespace if necessary.  Call this
// right before appending a token onto the output.
template<typename OutputConsumer>
void Minifier<OutputConsumer>::ChangeToken(int next_token) {
  if (whitespace_ != NO_WHITESPACE) {
    if (prev_token_ == '}') {
      output_.push_back('\n');
    } else if (IsExtendableOnRight(prev_token_) &&
               IsExtendableOnLeft(next_token)) {
      output_.push_back(whitespace_ == LINEBREAK ? '\n' : ' ');
    }
    whitespace_ = NO_WHITESPACE;
  }
  prev_token_ = next_token;
}

template<typename OutputConsumer>
void Minifier<OutputConsumer>::ConsumeComment() {
  DCHECK(index_ + 1 < input_.size());
  DCHECK(input_[index_] == '/');
  DCHECK(input_[index_ + 1] == '*');
  const int begin = index_;
  index_ += 2;
  while (index_ < input_.size()) {
    if (input_[index_] == '*' && Peek() == '/') {
      index_ += 2;
      const base::StringPiece comment = input_.substr(begin, index_ - begin);
      // We want to remove comments, but we need to preserve comments intended
      // as IE hacks to avoid breaking sites that rely on them.
      // See http://code.google.com/p/page-speed/issues/detail?id=432
      if (comment == "/**/") {
        ChangeToken(kCommentToken);
        output_.append(comment);
      } else if (whitespace_ == NO_WHITESPACE) {
        whitespace_ = SPACE;
      }
      return;
    }
    ++index_;
  }
  // If we reached EOF without the comment being closed, that's okay; just
  // don't include the partial comment in the output.
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
    } else if (ch == quote) {
      break;
    }
  }
  ChangeToken(kStringToken);
  output_.append(input_.substr(begin, index_ - begin));
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
    // Comments:
    else if (ch == '/' && Peek() == '*') {
      ConsumeComment();
    }
    // All other characters:
    else {
      ChangeToken(ch);
      output_.push_back(ch);
      ++index_;
    }
  }
}

}  // namespace

namespace pagespeed {

namespace css {

bool MinifyCss(const std::string& input, std::string* out) {
  Minifier<StringConsumer> minifier(input, out);
  return (minifier.GetOutput() != NULL);
}

bool GetMinifiedCssSize(const std::string& input, int* minified_size) {
  Minifier<SizeConsumer> minifier(input, NULL);
  SizeConsumer* output = minifier.GetOutput();
  if (output) {
    *minified_size = output->size();
    return true;
  } else {
    return false;
  }
}

}  // namespace css

}  // namespace pagespeed
