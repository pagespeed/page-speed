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

#ifndef PAGESPEED_HTML_HTML_TAG_H_
#define PAGESPEED_HTML_HTML_TAG_H_

#include <map>
#include <string>
#include <vector>

#include "base/basictypes.h"

namespace pagespeed {

namespace html {

class HtmlTag {
 public:
  HtmlTag();
  ~HtmlTag();

  // This is normal way to parse a tag.  You give a buffer that
  // starts with the < of an HTML Tag, and I read up to and including
  // the > that ends the tag.  RETURN a pointer past the > if I
  // successfully read a tag, or NULL if not.
  //
  // This always destroys current contents of "this", if any.
  //
  // For ease of use, this method will lowercase the tag name and attribute
  // names as it reads them (RFC 1866 section 3.2.3 specifies that tag and
  // attribute names are not case sensitive).
  const char* ReadTag(const char* begin, const char* end);

  // Search forward and read the next valid tag; return a pointer past the > of
  // the tag that was read, or NULL if there are no more tags.
  const char* ReadNextTag(const char* begin, const char* end);

  // Assuming this last tag read was an opening "foreign" tag (for example, a
  // style tag), search forward and read the matching closing tag; return a
  // pointer past the > of the tag that was read, or NULL if there is no such
  // closing tag.  This will ignore intervening tags, as a browser will.
  const char* ReadClosingForeignTag(const char* begin, const char* end);

  // Serialize the tag.
  std::string ToString() const;

  // Serialize the tag and append to the end of the string.
  void AppendTagToString(std::string* out) const;

  // Get the tag name.
  const std::string& tagname() const { return tag_name_; };

  // Get the tag name, but without the leading slash if this is a closing tag.
  const std::string GetBaseTagName() const;

  // true iff the tag ends with a slash: "<IMG />" (except </> is not empty)
  bool IsEmptyElement() const { return tag_type_ == SELF_CLOSING_TAG; }

  // true iff the tag begins with a slash: "</BODY>"
  bool IsEndTag() const { return tag_type_ == END_TAG; }

  // Return true iff this is a !DOCTYPE tag.
  bool IsDoctypeTag() const { return tag_type_ == DOCTYPE_TAG; }

  // Determine if any attributes are present.
  bool HasAnyAttrs() const { return !attr_names_.empty(); }

  // Determine if an attribute is present.
  bool HasAttr(const std::string& attr) const;

  // Add an attribute, but don't set a value for it.
  void AddAttr(const std::string& attr);

  // Remove an existing attribute.
  void ClearAttr(const std::string& attr);

  // Determine if an attribute is present and has a value.
  bool HasAttrValue(const std::string& attr) const;

  // Get the value of an attribute.
  const std::string& GetAttrValue(const std::string& attr) const;

  // Set the value of an existing attribute.
  void SetAttrValue(const std::string& attr, const std::string& value);

  // Remove an attribute's existing value (but not the attribute itself).
  void ClearAttrValue(const std::string& attr);

  // Sort the attributes by name.
  void SortAttributes();

 private:
  enum TagType { NEITHER_TAG, START_TAG, END_TAG, SELF_CLOSING_TAG,
                 COMMENT_TAG, DOCTYPE_TAG };

  typedef std::vector<std::string> AttrNames;
  typedef std::map<std::string, std::string> AttrMap;

  TagType tag_type_;
  std::string tag_name_;
  AttrNames attr_names_;
  AttrMap attr_map_;

  DISALLOW_COPY_AND_ASSIGN(HtmlTag);
};

}  // namespace html

}  // namespace pagespeed

#endif  // PAGESPEED_HTML_HTML_TAG_H_
