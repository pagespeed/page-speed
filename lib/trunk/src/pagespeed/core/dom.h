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

// DOM access API

#ifndef PAGESPEED_CORE_DOM_H_
#define PAGESPEED_CORE_DOM_H_

#include "base/basictypes.h"

#include <string>

namespace pagespeed {

class DomDocument;
class DomElement;
class DomElementVisitor;

/**
 * Document interface.
 */
class DomDocument {
 public:
  DomDocument();
  virtual ~DomDocument();

  // Return the URL that points to this document.
  virtual std::string GetDocumentUrl() const = 0;

  // Return the URL that is used as the base for relative URLs appearing in
  // this document.  Usually this is the same as the document URL, but it can
  // be changed with a <base> tag.
  virtual std::string GetBaseUrl() const;

  // Visit the elements within this document in pre-order (that is, always
  // visit a parent before visiting its children).
  virtual void Traverse(DomElementVisitor* visitor) const = 0;

  // Resolve a possibly-relative URI using this document's base URL.
  std::string ResolveUri(const std::string& uri) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(DomDocument);
};

/**
 * Element interface.
 */
class DomElement {
 public:
  DomElement();
  virtual ~DomElement();

  // Builds a new document instance for an IFrame's contents document.
  // It is up to the caller to dispose of this object once processing
  // is done.
  //
  // @return IFrame subdocument if the current node is an IFrame, else NULL.
  virtual DomDocument* GetContentDocument() const = 0;

  // Node type string.
  // Implementations must ensure that the contents of this string is
  // always UPPERCASE.
  virtual std::string GetTagName() const = 0;

  // @param name attribute name
  // @param attr_value output parameter to hold attribute value
  // @return true if the node has an attribute with that name.
  virtual bool GetAttributeByName(const std::string& name,
                                  std::string* attr_value) const;

  // Gets properties of the node object whose values are strings.
  //
  // @param name property name
  // @param property_value output parameter to hold property value
  // @return true if the node has a property with that name.
  virtual bool GetStringPropertyByName(const std::string& name,
                                       std::string* property_value) const;

  // Like GetStringPropertyByName, but returns integer values.
  //
  // @param name property name
  // @param property_value output parameter to hold property value
  // @return true if the node has a property with that name.
  virtual bool GetIntPropertyByName(const std::string& name,
                                    int* property_value) const;

  // @param name css property name
  // @param property_value output parameter to hold the css property value
  // @return true if the node has a css property with that name.
  virtual bool GetCSSPropertyByName(const std::string& name,
                                    std::string* property_value) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(DomElement);
};

class DomElementVisitor {
 public:
  DomElementVisitor();
  virtual ~DomElementVisitor();
  virtual void Visit(const DomElement& node) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(DomElementVisitor);
};

}  // namespace pagespeed

#endif  // PAGESPEED_CORE_DOM_H_
