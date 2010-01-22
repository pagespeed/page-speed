// Copyright 2008 Google Inc. All Rights Reserved.
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

// We do following things to reduce the size of an HTML document:
//
// 1. Remove the quotes around attribute values if possible, like
//    <div id="name"> (This is done by HtmlTag).
// 2. Collapse whitespaces between tags.
// 3. Remove comments (except those in a given whitelist).
// 4. Remove optional tags like </li>.
// 5. Lowercase tag names.
// 6. Remove attributes with default values like <input type=text>.
// 7. Simplify attributes with only one possible value, for example
//    <option selected=selected> can be written as <option selected>.
// 8. Sort attributes in a tag for better compression.
// 9. Remove comments and whitespaces in CSS.
// 10. Remove comments and whitespaces in JavaScript.
// 11. Use original attribute strings instead of unescaping and escaping them.
//     (So "'" will not be expanded to "&#39;".)
//
// NOTE: The modification to attributes may break pages with JavaScript
// if the code looks for the attributes that have been removed or modified.

#include "pagespeed/html/html_compactor.h"

#include <map>
#include <string>
#include <vector>

#include "base/logging.h"
#include "base/string_util.h"
#include "pagespeed/cssmin/cssmin.h"
#include "third_party/jsmin/cpp/jsmin.h"

namespace {

// A tag can be removed if it is in the kOptionalList.
//
// "/td" is removed from the list because IE will display different layout for
// the following two cases:
//   <table><tr><td><small>111</small></td> </table>
// and
//   <table><tr><td><small>111</small> </table>
// After removing the </td> tag, the space causes the height of the cell to
// increase because the font size of the space is larger than those in the
// "small" tag.
const char* const kOptionalList[] = {
  "!--", "html", "head", "/head", "/body", "/html", "/li", "/dt",
  "/dd", "/p", "/optgroup", "/option", "/colgroup", "/thead",
  "/tbody", "/tfoot", "/tr", "/th"
};

// An attribute can be removed from a tag if its name and value is in
// kDefaultList. If attr_value is NULL, it means matching just attribute name
// is enough. The list is derived from <http://www.w3.org/TR/html4/loose.dtd>.

struct TagAttrValue {
  const char* tag;
  const char* attr_name;
  const char* attr_value;
};

const TagAttrValue kDefaultList[] = {
  {"script", "language", NULL},
  {"script", "type", NULL},
  {"style", "type", NULL},
  {"br", "clear", "none"},
  {"a", "shape", "rect"},
  {"area", "shape", "rect"},
  {"param", "valuetype", "data"},
  {"form", "method", "get"},
  {"form", "enctype", "application/x-www-form-urlencoded"},
  {"input", "type", "text"},
  {"button", "type", "submit"},
  {"colgroup", "span", "1"},
  {"col", "span", "1"},
  {"th", "rowspan", "1"},
  {"th", "colspan", "1"},
  {"td", "rowspan", "1"},
  {"td", "colspan", "1"},
  {"frame", "frameborder", "1"},
  {"frame", "scrolling", "auto"},
  {"iframe", "frameborder", "1"},
  {"iframe", "scrolling", "auto"},
};

// An attribute can be simplified if it can have only one value, like
// <option selected=selected> can be simplified to <option selected>.
// The list is derived from <http://www.w3.org/TR/html4/loose.dtd>.

struct TagAttr {
  const char* tag;
  const char* attr_name;
};

const TagAttr kOneValueList[] = {
  {"area", "nohref"},
  {"img", "ismap"},
  {"object", "declare"},
  {"hr", "noshade"},
  {"dl", "compact"},
  {"ol", "compact"},
  {"ul", "compact"},
  {"dir", "compact"},
  {"menu", "compact"},
  {"input", "checked"},
  {"input", "disabled"},
  {"input", "readonly"},
  {"input", "ismap"},
  {"select", "multiple"},
  {"select", "disabled"},
  {"optgroup", "disabled"},
  {"option", "selected"},
  {"option", "disabled"},
  {"textarea", "disabled"},
  {"textarea", "readonly"},
  {"button", "disabled"},
  {"th", "nowrap"},
  {"td", "nowrap"},
  {"frame", "noresize"},
  {"script", "defer"},
};

// An attribute is listed in attrs of a TagEntry if it is a special attribute:
// DEFAULT: The attribute can be removed if it has the default value.
// ONE_VALUE: The attribute can have only one value and can be simplifed.
struct AttrEntry {
  const char* attr_name;
  const char* attr_value;
  enum { DEFAULT, ONE_VALUE } type;
};

// A tag has a TagEntry if it needs special processing:
// OPTIONAL: The tag is optional and can be removed.
// FOREIGN: The tag is one of "pre", "style", "script", or "textarea".
// SPECIAL_ATTR: The tag has special attributes in attrs. Each special attribute
//               is described by an AttrEntry above.

struct TagEntry {
  enum TagType { OPTIONAL = 1, FOREIGN = 2, SPECIAL_ATTR = 4 };
  int type;  // combination of TagType
  std::vector<AttrEntry> attrs;
};

typedef std::map<const std::string, TagEntry*> SpecialTagMap;

SpecialTagMap special_tags;

// Adds the type to the TagEntry with specified tag name.
// Creates a new TagEntry if there is not already one.
// Returns the TagEntry for the specified name.
TagEntry* AddTagEntry(const std::string& tag, TagEntry::TagType type) {
  if (special_tags.find(tag) == special_tags.end()) {
    TagEntry *t = new TagEntry();
    special_tags[tag] = t;
    t->type = type;
    return t;
  } else {
    TagEntry *t = special_tags[tag];
    t->type |= type;
    return t;
  }
}

void InitSpecialTagMap() {
  AddTagEntry("pre", TagEntry::FOREIGN);
  AddTagEntry("style", TagEntry::FOREIGN);
  AddTagEntry("script", TagEntry::FOREIGN);
  AddTagEntry("textarea", TagEntry::FOREIGN);

  for (int i = 0; i < arraysize(kOptionalList); i++) {
    AddTagEntry(kOptionalList[i], TagEntry::OPTIONAL);
  }

  for (int i = 0; i < arraysize(kDefaultList); i++) {
    TagEntry *t = AddTagEntry(kDefaultList[i].tag, TagEntry::SPECIAL_ATTR);
    AttrEntry a;
    a.attr_name = kDefaultList[i].attr_name;
    a.attr_value = kDefaultList[i].attr_value;
    a.type = AttrEntry::DEFAULT;
    t->attrs.push_back(a);
  }

  for (int i = 0; i < arraysize(kOneValueList); i++) {
    TagEntry *t = AddTagEntry(kOneValueList[i].tag, TagEntry::SPECIAL_ATTR);
    AttrEntry a;
    a.attr_name = kOneValueList[i].attr_name;
    a.attr_value = NULL;
    a.type = AttrEntry::ONE_VALUE;
    t->attrs.push_back(a);
  }
}

}  // namespace

namespace pagespeed {

namespace html {

HtmlCompactor::HtmlCompactor(std::string* output) : output_(output) {}

HtmlCompactor::~HtmlCompactor() {}

bool HtmlCompactor::CompactHtml(const std::string& input,
                                std::string* output) {
  static bool initialized = false;
  if (!initialized) {
    initialized = true;
    InitSpecialTagMap();
  }
  HtmlCompactor compactor(output);
  return compactor.CompactHtml(input);
}

// Compacts UTF-8 encoded HTML.
bool HtmlCompactor::CompactHtml(const std::string& input) {
  const char* begin = input.data();
  const char* end = begin + input.size();

  const char* p = begin;  // p points to the current char we are looking at.

  while (true) {
    // Find contiguous non-whitespace characters and output them as-is.
    const char* word_start = p;
    while (p < end && !isspace(*p) && *p != '<') {
      p++;
    }
    output_->append(word_start, p - word_start);
    if (p == end) {
      break;
    }
    if (*p == '<') {
      const char* next = cur_tag_.ReadTag(p, end);
      if (next) {
        p = ProcessTag(p, next, end);
      } else {
        output_->push_back(*p++);
      }

      continue;
    }
    // Find contiguous whitespace characters and replace them with one space.
    // (Unless the user set the option to disable compacting space.)
    const char* space_start = p;
    while (p < end && isspace(*p)) {
      p++;
    }
    if (output_->empty() || !isspace(*output_->rbegin())) {
      output_->push_back(*space_start);
    }
  }

  return true;
}

// Modifies a tag to reduce the size of its textual form.
// entry points to the special TagEntry. It is NULL if the tag is not special.
static void CompactTag(HtmlTag *tag, const TagEntry *entry) {
  // Handle special attributes.
  if (entry) {
    // We go through each possible special attributes for the TagEntry and see
    // if we have that attribute in the tag we are processing. If we have it, we
    // also try to match the value of the attribute with the one in AttrEntry if
    // necessary. If they do match, we do the corresponding modification to the
    // attribute in the tag.
    //
    // One alternative is to build entry->attrs as a hash table, and for each
    // attribute in a tag, we look up the hash table to see if it needs special
    // processing. It turns out to be slower because entry->attrs usually have
    // only 1 or 2 entries, and doing (# of attributes in a tag) hash lookup on
    // it is slower than just compare them with all the attributes.
    for (int i = 0; i < entry->attrs.size(); i++) {
      const AttrEntry& item = entry->attrs[i];
      const std::string attr_name(item.attr_name);
      if (!tag->HasAttr(attr_name)) {
        continue;
      }
      if (item.type == AttrEntry::DEFAULT) {
        // Remove attributes with default value.
        if (item.attr_value == NULL || !tag->HasAttrValue(attr_name) ||
            tag->GetAttrValue(attr_name) == std::string(item.attr_value)) {
          tag->ClearAttr(attr_name);
        }
      } else if (item.type == AttrEntry::ONE_VALUE &&
                 tag->HasAttrValue(attr_name)) {
        // Set attributes to no value.
        tag->ClearAttrValue(attr_name);
      }
    }
  }

  // Sort attributes.
  if (!tag->IsDoctypeTag()) {  // The "!doctype" tag cannot be reordered.
    tag->SortAttributes();
  }
}

// Processes the current tag.
// Removes comment/optional tags and compacts special/normal tags.
//
// The current tag is in the range [tag_begin, tag_end).
// all_end points to one past the last character of the document.
// If the current tag is foreign, processes all the way to the end tag.
// Returns the point after the current tag (or after the foreign end tag).
//
// According to <http://www.w3.org/TR/html401/appendix/notes.html#h-B.3.2.1>,
// the data should stop at "</" followed by a character [a-zA-Z], but my
// Firefox seems to stop only at matching closing tag (like </script>).
const char* HtmlCompactor::ProcessTag(const char* tag_begin,
                                      const char* tag_end,
                                      const char* all_end) {
  const std::string& tag_name = cur_tag_.tagname();

  // Respect !DOCTYPE tags.
  if (cur_tag_.IsDoctypeTag()) {
    cur_tag_.AppendTagToString(output_);
    return tag_end;
  }

  // Keep XML processing instruction tags intact, including quotes.
  DCHECK(tag_name.size() > 0);
  if (tag_name[0] == '?') {
    output_->append(tag_begin, tag_end - tag_begin);
    return tag_end;
  }

  SpecialTagMap::const_iterator p = special_tags.find(tag_name);
  const TagEntry* t = (p == special_tags.end()) ? NULL : p->second;
  // Skip if this is a comment or an "optional" tag, as long as it has no
  // attributes.
  if (t && (t->type & TagEntry::OPTIONAL) && !cur_tag_.HasAnyAttrs()) {
    // But keep it if this is a special IE tag.
    if (tag_name[0] == '!') {
      const char* marker = "<!--[";
      const int len = strlen(marker);
      if (tag_end - tag_begin >= len && !memcmp(tag_begin, marker, len)) {
        output_->append(tag_begin, tag_end - tag_begin);
      }
    }
    return tag_end;
  }
  // Check if this is a foreign tag (and not an
  // empty element like <style ... />).
  if (t && (t->type & TagEntry::FOREIGN) && !cur_tag_.IsEmptyElement()) {
    // Find the corresponding end tag.
    HtmlTag end_tag;
    const char* next;
    const char* p = tag_end;
    while (true) {
      p = (const char*)memchr(p, '<', all_end - p);
      if (!p) {
        break;
      }
      if (p + 1 < all_end && *(p + 1) != '/') {  // This is just optimization.
        p++;
        continue;
      }
      next = end_tag.ReadTag(p, all_end);
      if (next && end_tag.IsEndTag()) {
        // Try to match the end tag name and the start tag name.
        if (end_tag.GetBaseTagName() == tag_name) {
          break;
        }
      }
      ++p;
    }

    if (p) {
      // <script language="javascript">..........</script>......
      // ^tag_begin                    ^tag_end  ^p       ^next
      CompactTag(&cur_tag_, t);
      cur_tag_.AppendTagToString(output_);
      CompactForeign(tag_end, p);
      CompactTag(&end_tag, NULL);
      end_tag.AppendTagToString(output_);
      return next;
    }
    // If we cannot find the end tag, log it and treat it as a normal tag.
    std::string context(tag_begin, std::min<std::string::size_type>
                        (all_end - tag_begin, 40));
    LOG(INFO) << "Cannot find the end tag after [" << context << "]";
  }

  CompactTag(&cur_tag_, t);
  cur_tag_.AppendTagToString(output_);
  return tag_end;
}

// Compacts the content inside CSS/JS/PRE/TEXTAREA tags. The content is
// in the range [begin, end). The start tag is in cur_tag_.
void HtmlCompactor::CompactForeign(const char* begin, const char* end) {
  if (cur_tag_.tagname() == "style") {
    std::string input(begin, end - begin);
    std::string minified;
    if (cssmin::MinifyCss(input, &minified)) {
      output_->append(minified);
    } else {
      LOG(WARNING) << "Inline CSS minification failed.";
      output_->append(begin, end - begin);
    }
  } else if (cur_tag_.tagname() == "script") {
    std::string input(begin, end - begin);
    std::string minified;
    if (jsmin::MinifyJs(input, &minified)) {
      output_->append(minified);
    } else {
      LOG(WARNING) << "Inline JS minification failed.";
      output_->append(begin, end - begin);
    }
  } else {  // Keep the content intact for other tags.
    output_->append(begin, end - begin);
  }
}

}  // namespace html

}  // namespace pagespeed
