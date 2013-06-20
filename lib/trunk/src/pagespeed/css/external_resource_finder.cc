// Copyright 2011 Google Inc. All Rights Reserved.
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

#include "pagespeed/css/external_resource_finder.h"

#include "base/logging.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/string_util.h"
#include "pagespeed/core/uri_util.h"

using pagespeed::css::CssTokenizer;

namespace {

const char* kCssWhitespaceChars = " \t\r\n\f";
const char* kCommentStart = "/*";
const char* kCommentEnd = "*/";
const char* kCssImportDirective = "@import";
const char* kCssUrlDirective = "url(";
const size_t kCommentStartLen = strlen(kCommentStart);
const size_t kCommentEndLen = strlen(kCommentEnd);
const size_t kCssUrlDirectiveLen = strlen(kCssUrlDirective);

// Determines if the character is a valid character for a CSS
// identifier, as defined in the CSS 2.1 grammar. Note that our
// definition of an ident char is broadened a bit from what is
// specified in the grammar, and does not include multi-byte
// characters.
inline bool IsIdentChar(unsigned char candidate) {
  return
      candidate == '-' ||
      (candidate >= 'a' && candidate <= 'z') ||
      (candidate >= 'A' && candidate <= 'Z') ||
      (candidate >= '0' && candidate <= '9') ||
      candidate == '-' || candidate == '_' || candidate == '.';
}

}  // namespace

namespace pagespeed {

namespace css {


void FindExternalResourcesInCssResource(
    const Resource& resource,
    std::set<std::string>* external_resource_urls) {
  if (resource.GetResourceType() != pagespeed::CSS) {
    LOG(DFATAL) << "Non-CSS resource passed to"
                << " FindExternalResourcesInCssResource.";
    return;
  }
  FindExternalResourcesInCssBlock(
      resource.GetRequestUrl(), resource.GetResponseBody(),
      external_resource_urls);
}

void FindExternalResourcesInCssBlock(
    const std::string& resource_url, const std::string& css_body,
    std::set<std::string>* external_resource_urls) {
  std::string body;

  // Make our search easier by removing comments. We could be more
  // efficient by attempting to skip over comments as we walk the
  // string, but this would complicate the logic. It's simpler to
  // remove comments first, then iterate over the string.
  RemoveCssComments(css_body, &body);
  CssTokenizer tokenizer(body);
  std::string token;
  CssTokenizer::CssTokenType type;
  std::string url;
  while (tokenizer.GetNextToken(&token, &type)) {
    url.clear();
    if (type == CssTokenizer::URL) {
      url = token;
    } else if (type == CssTokenizer::IDENT &&
               pagespeed::string_util::LowerCaseEqualsASCII(
                   token, kCssImportDirective)) {
      // @import can contain either a url, e.g. "url('foo.css')" or a
      // plain string, e.g. "foo.css". Either way, it will be the
      // immediate next token.
      if (tokenizer.GetNextToken(&token, &type) &&
          (type == CssTokenizer::URL || type == CssTokenizer::STRING)) {
        url = token;
      }
    }

    if (!url.empty()) {
      // Resolve the URI relative to its parent stylesheet.
      std::string resolved_url =
          pagespeed::uri_util::ResolveUri(url, resource_url);
      if (resolved_url.empty()) {
        LOG(INFO) << "Unable to ResolveUri " << url;
      } else {
        external_resource_urls->insert(resolved_url);
      }
    }
  }
}

// The CSS 2.1 Specification section on comments
// (http://www.w3.org/TR/CSS21/syndata.html#comments) notes:
//
//  Comments begin with the characters "/*" and end with the
//  characters "*/". ... CSS also allows the SGML comment delimiters
//  ("<!--" and "-->") in certain places defined by the grammar, but
//  they do not delimit CSS comments.
//
// Thus we remove /* */ comments, but we do not scan for or remove
// SGML comments, since these are supported only for very old user
// agents. If many web pages do use such comments, we may need to add
// support for them.
void RemoveCssComments(const std::string& in, std::string* out) {
  size_t comment_start = 0;
  while (true) {
    const size_t previous_start = comment_start;
    comment_start = in.find(kCommentStart, comment_start);
    if (comment_start == in.npos) {
      // No more comments. Append to end of string and we're done.
      out->append(in, previous_start, in.length() - previous_start);
      break;
    }

    // Append the content before the start of the comment.
    out->append(in, previous_start, comment_start - previous_start);

    const size_t comment_end =
        in.find(kCommentEnd, comment_start + kCommentStartLen);
    if (comment_end == in.npos) {
      // Unterminated comment. We're done.
      break;
    }
    comment_start = comment_end + kCommentEndLen;
  }
}

CssTokenizer::CssTokenizer(const std::string& css_body)
    : css_body_(css_body),
      index_(0) {
}

bool CssTokenizer::GetNextToken(std::string* out_token,
                                CssTokenType* out_type) {
  out_token->clear();

  if (index_ >= css_body_.size()) {
    return false;
  }

  // skip over leading whitespace
  index_ = css_body_.find_first_not_of(kCssWhitespaceChars, index_);
  if (index_ == std::string::npos) {
    return false;
  }

  const size_t prev_index = index_;

  // First try to extract a URL, then a CSS identifier, and finally a
  // string. It's important that we check for URL first since the CSS
  // url prefix "url" is also a valid CSS identifier.
  if (TakeUrl(out_token)) {
    *out_type = URL;
    return true;
  }
  if (index_ == prev_index && TakeIdent(out_token)) {
    *out_type = IDENT;
    return true;
  }
  if (index_ == prev_index && TakeString(out_token)) {
    *out_type = STRING;
    return true;
  }
  if (index_ != prev_index) {
    // One of the Take functions didn't find a valid token, but did
    // consume characters. Emit the consumed characters as an invalid
    // token.
    out_token->assign(css_body_, prev_index, index_ - prev_index);
    *out_type = INVALID;
  } else {
    // We didn't find an ident or a string, so the token is likely a
    // one character separator.
    out_token->assign(css_body_, index_, 1);
    ++index_;
    *out_type = SEPARATOR;
  }
  return true;
}

bool CssTokenizer::TakeString(std::string* out_token) {
  return TakeString(out_token, &index_);
}

bool CssTokenizer::TakeString(std::string* out_token, size_t *inout_index) {
  if (*inout_index >= css_body_.length()) {
    return false;
  }
  const char start_quote = css_body_[*inout_index];
  if (start_quote != '"' && start_quote != '\'') {
    return false;
  }

  size_t next_token = *inout_index + 1;
  bool done = false;
  while (!done && next_token < css_body_.size()) {
    unsigned char candidate = css_body_[next_token];
    if (candidate == start_quote) {
      done = true;
      break;
    }
    switch (candidate) {
      case '\\':
        next_token += ConsumeEscape(next_token, out_token);
        break;

      case '\r':
      case '\n':
      case '\f':
        done = true;
        break;

      default:
        out_token->push_back(candidate);
        break;
    }
    if (done) {
      break;
    }
    ++next_token;
  }

  if (next_token < css_body_.length()) {
    if (css_body_[next_token] == start_quote) {
      ++next_token;
    }
  } else {
    next_token = css_body_.length();
  }
  *inout_index = next_token;
  return true;
}

bool CssTokenizer::TakeUrl(std::string* out_token) {
  if (index_ + kCssUrlDirectiveLen >= css_body_.length()) {
    return false;
  }
  std::string possible_url_directive(
      css_body_.c_str() + index_, kCssUrlDirectiveLen);
  if (!pagespeed::string_util::LowerCaseEqualsASCII(
          possible_url_directive,
          kCssUrlDirective)) {
    // Doesn't start with "url(", so it can't be a URL token.
    return false;
  }

  // Skip over whitespace.
  size_t next_token = css_body_.find_first_not_of(
      kCssWhitespaceChars, index_ + kCssUrlDirectiveLen);
  if (next_token == std::string::npos) {
    return false;
  }

  // First, try to scan for a quoted string inside the "url(".
  if (TakeString(out_token, &next_token)) {
    // Found a quoted string. Now skip over whitespace after it.
    next_token = css_body_.find_first_not_of(kCssWhitespaceChars, next_token);
    if (next_token == std::string::npos) {
      // We found a quoted URL but only whitespace after the URL,
      // indicating a premature EOF. CSS parsers don't parse such URLs
      // but we do want to consume the characters, so update index_
      // and return false.
      index_ = css_body_.length();
      return false;
    }
    if (css_body_[next_token] != ')') {
      // The next non-whitespace character after the quoted string was
      // not a closing parentheses. This is an error case. In this
      // case, WebKit will search for a closing parentheses, and
      // ignore all content up to that point. We do the same.
      next_token = css_body_.find(')', next_token);
      if (next_token != std::string::npos) {
        index_ = next_token + 1;
      } else {
        // There was no closing parentheses, so consume all remaining
        // characters.
        index_ = css_body_.length();
      }
      return false;
    }
    index_ = next_token + 1;
    return true;
  }

  // If we were unable to find a quoted string, fall back to taking
  // the entire unquoted string inside of the parentheses.
  size_t close_paren = css_body_.find(')', index_ + kCssUrlDirectiveLen);
  if (close_paren == std::string::npos) {
    return false;
  }
  size_t url_start = index_ + kCssUrlDirectiveLen;
  out_token->assign(css_body_, url_start, close_paren - url_start);
  pagespeed::string_util::TrimWhitespaceASCII(
      *out_token, pagespeed::string_util::TRIM_ALL, out_token);
  index_ = close_paren + 1;
  return true;
}

bool CssTokenizer::TakeIdent(std::string* out_token) {
  if (index_ >= css_body_.length()) {
    return false;
  }
  // We use a mini state machine to identify "ident" tokens. Our
  // definition of an ident token is a broadened version of the
  // definition in the CSS 2.1 grammar
  // (http://www.w3.org/TR/CSS21/grammar.html).
  enum IdentState {
    OPTIONAL_FIRST_CHAR,
    REQUIRED_IDENT_CHAR,
    OPTIONAL_IDENT_CHARS,
    OPTIONAL_LAST_CHAR,
    DONE,
    ERR,
  };

  size_t next_token = index_;
  IdentState s = OPTIONAL_FIRST_CHAR;
  while (s != ERR && s != DONE && next_token < css_body_.size()) {
    unsigned char candidate = css_body_[next_token];
    switch (s) {
      case OPTIONAL_FIRST_CHAR:
        if (candidate == '-' ||
            candidate == '@' ||
            candidate == '!' ||
            candidate == '.' ||
            candidate == '#') {
          ++next_token;
        }
        s = REQUIRED_IDENT_CHAR;
        break;

      case REQUIRED_IDENT_CHAR:
        if (IsIdentChar(candidate)) {
          s = OPTIONAL_IDENT_CHARS;
          ++next_token;
        } else {
          // A valid identifier must contain at least one ident character.
          s = ERR;
        }
        break;

      case OPTIONAL_IDENT_CHARS:
        if (IsIdentChar(candidate)) {
          ++next_token;
        } else {
          s = OPTIONAL_LAST_CHAR;
        }
        break;

      case OPTIONAL_LAST_CHAR:
        // These characters can optionally appear at the end of
        // various CSS tokens.
        if (candidate == '%') {
          ++next_token;
        }
        s = DONE;
        break;

      case DONE:
        break;

      default:
        LOG(DFATAL) << "Unhandled IdentState " << s;
        break;
    }
  }

  // Special case: if we got to EOF but we consumed enough characters
  // to have a valid identifier, advance the state to DONE.
  if (next_token == css_body_.size() && s != ERR) {
    if (s > REQUIRED_IDENT_CHAR) {
      s = DONE;
    }
  }
  const bool success = (s == DONE);
  if (success) {
    out_token->assign(css_body_, index_, next_token - index_);
    index_ = next_token;
  }
  return success;
}

size_t CssTokenizer::ConsumeEscape(size_t next_token, std::string* out_token) {
  ++next_token;
  const size_t remaining = css_body_.size() - next_token;
  const unsigned char first = css_body_[next_token];
  if (remaining >= 2) {
    const unsigned char second = css_body_[next_token + 1];
    if (first == '\r' && second == '\n') {
      // Silently consume the CR LF, per the CSS 2 spec.
      return 2;
    }
  }
  if (remaining >= 1) {
    switch (first) {
      case '\r':
      case '\n':
        // Silently consume these escaped characters, per the CSS 2 spec.
        return 1;

      default:
        break;
    }
    out_token->push_back(first);
    return 1;
  }

  // Nothing to consume.
  return 0;
}

}  // namespace rules

}  // namespace pagespeed
