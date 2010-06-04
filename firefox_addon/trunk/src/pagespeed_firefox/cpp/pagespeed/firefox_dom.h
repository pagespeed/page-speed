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

#include "base/basictypes.h"

#include "pagespeed/core/dom.h"

#include "nsCOMPtr.h"

class nsIDOMDocument;
class nsIDOMElement;

namespace pagespeed {

class FirefoxDocument : public DomDocument {
 public:
  FirefoxDocument(nsIDOMDocument* document);

  virtual std::string GetDocumentUrl() const;

  virtual std::string GetBaseUrl() const;

  virtual void Traverse(DomElementVisitor* visitor) const;

 private:
  nsCOMPtr<nsIDOMDocument> document_;

  DISALLOW_COPY_AND_ASSIGN(FirefoxDocument);
};

class FirefoxElement : public DomElement {
 public:
  FirefoxElement(nsIDOMElement* element);
  virtual DomDocument* GetContentDocument() const;
  virtual std::string GetTagName() const;
  virtual bool GetAttributeByName(const std::string& name,
                                  std::string* attr_value) const;

  Status GetActualWidth(int* out_width) const;
  Status GetActualHeight(int* out_height) const;
  Status HasWidthSpecified(bool* out_width_specified) const;
  Status HasHeightSpecified(bool* out_height_specified) const;

 private:
  bool GetClientWidthOrHeight(const std::string& name,
                              int* out_property_value) const;

  bool GetCSSPropertyByName(const std::string& name,
                            std::string* out_property_value) const;

  nsCOMPtr<nsIDOMElement> element_;

  DISALLOW_COPY_AND_ASSIGN(FirefoxElement);
};

}  // namespace pagespeed
