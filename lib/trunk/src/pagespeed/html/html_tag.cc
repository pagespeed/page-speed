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

#include "pagespeed/html/html_tag.h"

#include <algorithm>  // for sort
#include <string>

#include "base/logging.h"
#include "base/string_util.h"

namespace {

bool IsTagnameStart(char c) {
  return (('A' <= c && c <= 'Z') ||
          ('a' <= c && c <= 'z') ||
          c == '/' || c == '!' || c == '?');
}

bool IsTagnameRest(char c) {
  return !isspace(c) && c != '\0' && c != '>' && c != '/';
}

bool IsAttrRest(char c) {
  return !isspace(c) && c != '\0' && c != '>' && c != '=';
}

// Determine if a character needs to be quoted in an HTML attribute value.
bool NeedsQuote(unsigned char c) {
  return (c <= ' ' || '\x7F' <= c ||
          c == '"' || c == '\'' || c == '>' || c == '`');
}

// Returns 0 if the input string does not need quotes, otherwise returns
// " or ' depending on whether one has already appeared in the string.
// If both " and ' appear in the input, 0 is returned because that can
// be read by ReadAttributeValuePair(), IE6, and Firefox.
char PickQuote(const std::string& str) {
  if (str.empty()) {
    return '"';  // Empty values always need quotes.
  }

  bool needs_quotes = false;
  bool saw_squote = false;
  bool saw_dquote = false;

  for (std::string::const_iterator i = str.begin(), end = str.end();
       i != end; ++i) {
    const char c = *i;
    if (NeedsQuote(c)) {
      needs_quotes = true;
      if (c == '\'') {
        saw_dquote = true;
      } else if (c == '"') {
        saw_dquote = true;
      }
    }
  }

  return (!needs_quotes ? '\0' :
          !saw_dquote ? '"' :
          !saw_squote ? '\'' : '\0');
}

const char* ReadComment(const char* begin, const char* end) {
  const char* p = begin;
  while (p + 3 < end && strncmp(p, "-->", 3)) {
    ++p;
  }
  return p + 3;
}

}  // namespace

namespace pagespeed {

namespace html {

HtmlTag::HtmlTag() : tag_type_(HtmlTag::NEITHER_TAG) {}

HtmlTag::~HtmlTag() {}

const std::string HtmlTag::GetBaseTagName() const {
  if (tag_type_ == END_TAG) {
    DCHECK(tag_name_[0] == '/');
    return tag_name_.substr(1);
  } else {
    return tag_name_;
  }
}

bool HtmlTag::HasAttr(const std::string& attr) const {
  for (AttrNames::const_iterator i = attr_names_.begin(),
           end = attr_names_.end(); i != end; ++i) {
    if (*i == attr) {
      return true;
    }
  }
  return false;
}

void HtmlTag::AddAttr(const std::string& attr) {
  DCHECK(!HasAttr(attr)) << "attr already exists: " << attr;
  attr_names_.push_back(attr);
}

void HtmlTag::ClearAttr(const std::string& attr) {
  if (HasAttrValue(attr)) {
    ClearAttrValue(attr);
  }
  for (AttrNames::iterator i = attr_names_.begin(), end = attr_names_.end();
       i != end; ++i) {
    if (*i == attr) {
      attr_names_.erase(i);
      return;
    }
  }
  DCHECK(false) << "no such attr: " << attr;;
}

bool HtmlTag::HasAttrValue(const std::string& attr) const {
  return attr_map_.find(attr) != attr_map_.end();
}

const std::string& HtmlTag::GetAttrValue(const std::string& attr) const {
  AttrMap::const_iterator i = attr_map_.find(attr);
  CHECK(i != attr_map_.end()) << "no such attr: " << attr;
  return i->second;
}

void HtmlTag::SetAttrValue(const std::string& attr, const std::string& value) {
  DCHECK(HasAttr(attr)) << "no such attr: " << attr;
  attr_map_[attr] = value;
}

void HtmlTag::ClearAttrValue(const std::string& attr) {
  AttrMap::iterator i = attr_map_.find(attr);
  CHECK(i != attr_map_.end()) << "no such attr: " << attr;
  attr_map_.erase(i);
}

void HtmlTag::SortAttributes() {
  std::sort(attr_names_.begin(), attr_names_.end());
}

// This macro is used only in ReadTag, and we #undef if afterwards.
#define READTAG_SKIP(pred) do {                     \
    while (true) {                                  \
      if (p >= end) {                               \
        return NULL; /* fail if we reach the end */ \
      } else if (pred(*p)) {                        \
        ++p;                                        \
      } else {                                      \
        break;                                      \
      }                                             \
    }                                               \
  } while (false)

const char* HtmlTag::ReadTag(const char* begin, const char* end) {
  if (end - begin < 3 || *begin != '<') {  // <x> is the minimal tag
    return NULL; // not a tag
  }

  if (end - begin >= 4 && !strncmp(begin + 1, "!--", 3)) {
    tag_name_ = "!--";
    tag_type_ = COMMENT_TAG;
    return ReadComment(begin + 2, end);
  }

  // Read the tag name.
  const char* tag_name_start = begin + 1;  // skip opening '<'
  const char* p = tag_name_start;
  if (!IsTagnameStart(*p)) {
    return NULL;
  }
  ++p;
  READTAG_SKIP(IsTagnameRest);
  tag_name_.assign(tag_name_start, p - tag_name_start);
  StringToLowerASCII(&tag_name_);

  const bool is_doctype = (tag_name_ == "!doctype");
  if (is_doctype) {
    tag_type_ = DOCTYPE_TAG;
  }

  // Read the attributes.
  attr_names_.clear();
  attr_map_.clear();
  while (true) {
    READTAG_SKIP(isspace);

    // Are we at the end of the tag?
    if (*p == '>') {
      if (!is_doctype) {
        tag_type_ = (*tag_name_start == '/' ? END_TAG : START_TAG);
      }
      return p + 1;
    } else if (p + 1 < end && p[0] == '/' && p[1] == '>') {
      if (!is_doctype) {
        tag_type_ = SELF_CLOSING_TAG;
      }
      return p + 2;
    }

    // Read the attribute name.
    const char* attr_name_start = p;
    if (is_doctype) {
      const char first_char = *attr_name_start;
      if (first_char == '"' || first_char == '\'') {
        ++p;
        READTAG_SKIP(first_char !=);
        ++p;
        const std::string str(attr_name_start, p - attr_name_start);
        if (HasAttr(str)) {
          LOG(WARNING) << "duplicated " << str << " attribute in "
                       << tag_name_ << " tag";
        } else {
          AddAttr(str);
        }
        continue;
      }
    }
    READTAG_SKIP(IsAttrRest);
    if (p == attr_name_start) {
      return NULL;
    }
    std::string attr_name(attr_name_start, p - attr_name_start);
    StringToLowerASCII(&attr_name);

    // Read the = that separates attribute name from attribute value.
    READTAG_SKIP(isspace);
    if (*p != '=') {  // we don't have a value
      if (HasAttr(attr_name)) {
        LOG(WARNING) << "duplicated " << attr_name << " attribute in "
                     << tag_name_ << " tag";
      } else {
        AddAttr(attr_name);
      }
      continue;  // done with this attr/value pair
    }
    ++p;
    READTAG_SKIP(isspace);

    // Read the attribute value.
    const char* attr_value_start;
    const char* attr_value_end;
    const char first_char = *p;
    if (first_char == '"' || first_char == '\'') {
      ++p;
      attr_value_start = p;
      READTAG_SKIP(first_char !=);
      attr_value_end = p;
      ++p;
    } else {
      attr_value_start = p;
      READTAG_SKIP(!NeedsQuote);
      attr_value_end = p;
    }
    // Ignore this attribute if we already have an attribute of the same name.
    // E.g. <foo bar=baz bar=quux> turns into <foo bar=baz>.
    // Unfortunately, I couldn't find anything specified in an RFC about how to
    // handle repeated attributes like this, but Firefox and Chrome both seem to
    // ignore all but the first value given for the attribute, so that's what
    // HtmlTag does too.  (mdsteele)
    if (HasAttr(attr_name)) {
      LOG(WARNING) << "duplicated " << attr_name << " attribute in "
                   << tag_name_ << " tag";
    } else {
      const std::string attr_value(attr_value_start, attr_value_end);
      AddAttr(attr_name);
      SetAttrValue(attr_name, attr_value);
    }
  }
}

#undef READTAG_SKIP

const char* HtmlTag::ReadNextTag(const char* begin, const char* end) {
  while (begin < end) {
    while (begin < end && *begin != '<') {
      ++begin;
    }

    const char* result = ReadTag(begin, end);
    if (result == NULL) {
      ++begin;
    } else {
      return result;
    }
  }

  return NULL;
}

const char* HtmlTag::ReadClosingForeignTag(const char* begin,
                                           const char* end) {
  DCHECK(tag_type_ == START_TAG);
  const std::string base_tag_name(tag_name_);

  while (begin < end) {
    while (begin < end && *begin != '<') {
      ++begin;
    }

    if (begin + 1 < end && *(begin + 1) == '/') {
      const char* tag_end = ReadTag(begin, end);
      if (tag_end != NULL && IsEndTag() && GetBaseTagName() == base_tag_name) {
        return tag_end;
      }
    }

    ++begin;
  }

  return NULL;
}

std::string HtmlTag::ToString() const {
  std::string output;
  AppendTagToString(&output);
  return output;
}

void HtmlTag::AppendTagToString(std::string* out) const {
  DCHECK(out != NULL);
  out->push_back('<');
  out->append(tag_name_);

  for (AttrNames::const_iterator i = attr_names_.begin(),
           end = attr_names_.end(); i != end; ++i) {
    const std::string& attr = *i;
    out->push_back(' ');
    out->append(attr);
    if (HasAttrValue(attr)) {
      const std::string& value = GetAttrValue(attr);
      const char quote = PickQuote(value);
      out->push_back('=');
      if (quote) {
        out->push_back(quote);
      }
      out->append(value);
      if (quote) {
        out->push_back(quote);
      }
    }
  }

  if (IsEmptyElement()) {
    out->append(" />");  // Always add a space
  } else {
    out->push_back('>');
  }
}

}  // namespace html

}  // namespace pagespeed
