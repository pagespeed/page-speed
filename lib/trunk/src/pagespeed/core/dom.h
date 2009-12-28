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

  // Visit the elements within this document.
  virtual void Traverse(DomElementVisitor* visitor) const = 0;

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

  // Gets the absolute URL of the resource associated with this
  // node. For instance, the URL referenced in the 'src' attribute of
  // an img or script tag, or the 'href' attibute of a link tag. Does
  // not return resources referenced in the 'href' attribute of an 'a'
  // tag, since those resources are not part of the page that this
  // DomElement belongs to.
  //
  // @param src output parameter to hold the URL of the resource.
  // @return true if the node has an associated resource URL.
  virtual bool GetResourceUrl(std::string* src) const = 0;

  // @param name attribute name
  // @param attr_value output parameter to hold attribute value
  // @return true if the node has an attribute with that name.
  virtual bool GetAttributeByName(const std::string& name,
                                  std::string* attr_value) const = 0;

  // @param name css property name
  // @param property_value output parameter to hold the css property value
  // @return true if the node has an attribute with that name.
  virtual bool GetCSSPropertyByName(const std::string& name,
                                    std::string* property_value) const = 0;
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
