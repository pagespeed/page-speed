// Copyright 2010 Google Inc.
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

#include "pagespeed_chromium/npapi_dom.h"

#include <string>

#include "base/basictypes.h"
#include "base/logging.h"
#include "base/string_number_conversions.h"

namespace pagespeed_chromium {

namespace {

// Try to get a object-valued property with the given name from the given
// NPObject.  If successful, return the NPObject pointer; otherwise return
// NULL.  The caller is responsible for calling NPN_ReleaseObject on the result
// (if it is non-NULL) when finished with it.
NPObject* GetObjectProperty(NPP npp, NPObject* object, const char* name) {
  NPObject* rval = NULL;
  NPVariant result;
  if (NPN_GetProperty(npp, object, NPN_GetStringIdentifier(name), &result)) {
    if (NPVARIANT_IS_OBJECT(result)) {
      rval = NPVARIANT_TO_OBJECT(result);
      NPN_RetainObject(rval);
    }
    NPN_ReleaseVariantValue(&result);
  }
  return rval;
}

// Try to get a string-valued property with the given name from the given
// NPObject.  If successful, put the value into the output string and return
// true; otherwise, return false.
bool GetStringProperty(NPP npp, NPObject* object, const char* name,
                       std::string* output) {
  bool rval = false;
  NPVariant result;
  if (NPN_GetProperty(npp, object, NPN_GetStringIdentifier(name), &result)) {
    if (NPVARIANT_IS_STRING(result)) {
      const NPString& url_NPString = NPVARIANT_TO_STRING(result);
      output->assign(url_NPString.UTF8Characters, url_NPString.UTF8Length);
      rval = true;
    }
    NPN_ReleaseVariantValue(&result);
  }
  return rval;
}

// Like GetStringProperty, but assumes that we'll be successful, and simply
// returns the string value, and LOG(DFATAL) if it fails.
std::string DemandStringProperty(NPP npp, NPObject* object, const char* name) {
  std::string rval;
  if (!GetStringProperty(npp, object, name, &rval)) {
    LOG(DFATAL) << "Could not get " << name << " property";
  }
  return rval;
}

class NpapiDocument : public pagespeed::DomDocument {
 public:
  NpapiDocument(NPP npp, NPObject* document);
  virtual ~NpapiDocument();

  // Return the URL that points to this document.
  virtual std::string GetDocumentUrl() const;

  // Return the URL that is used as the base for relative URLs appearing in
  // this document.  Usually this is the same as the document URL, but it can
  // be changed with a <base> tag.
  virtual std::string GetBaseUrl() const;

  // Visit the elements within this document in pre-order (that is, always
  // visit a parent before visiting its children).
  virtual void Traverse(pagespeed::DomElementVisitor* visitor) const;

 private:
  const NPP npp_;
  NPObject* const document_;

  DISALLOW_COPY_AND_ASSIGN(NpapiDocument);
};

class NpapiElement : public pagespeed::DomElement {
 public:
  NpapiElement(NPP npp, NPObject* element);
  virtual ~NpapiElement();

  // Builds a new document instance for an IFrame's contents document.
  // It is up to the caller to dispose of this object once processing
  // is done.
  //
  // @return IFrame subdocument if the current node is an IFrame, else NULL.
  virtual pagespeed::DomDocument* GetContentDocument() const;

  // Node type string.
  // Implementations must ensure that the contents of this string is
  // always UPPERCASE.
  virtual std::string GetTagName() const;

  // @param name attribute name
  // @param attr_value output parameter to hold attribute value
  // @return true if the node has an attribute with that name.
  virtual bool GetAttributeByName(const std::string& name,
                                  std::string* attr_value) const;

 private:
  const NPP npp_;
  NPObject* const element_;

  DISALLOW_COPY_AND_ASSIGN(NpapiElement);
};

NpapiDocument::NpapiDocument(NPP npp, NPObject* document)
    : npp_(npp), document_(document) {
  NPN_RetainObject(document_);
}

NpapiDocument::~NpapiDocument() {
  NPN_ReleaseObject(document_);
}

std::string NpapiDocument::GetDocumentUrl() const {
  return DemandStringProperty(npp_, document_, "URL");
}

std::string NpapiDocument::GetBaseUrl() const {
  return DemandStringProperty(npp_, document_, "baseURI");
}

// This implementation is not browser-specific; however, we use the
// firstElementChild and nextElementSibling properties, which are part of DOM4
// (http://dev.w3.org/2006/webapi/DOM4Core/DOM4Core.html), so it may not work
// in older browsers (but it'll be fine in Chrome).
void NpapiDocument::Traverse(pagespeed::DomElementVisitor* visitor) const {
  NPObject* element = GetObjectProperty(npp_, document_, "documentElement");
  while (element) {
    // Visit the element.
    const NpapiElement chromium_element(npp_, element);
    visitor->Visit(chromium_element);
    // Check for a child.
    NPObject* next = GetObjectProperty(npp_, element, "firstElementChild");
    // If no children, check for a sibling.
    if (!next) {
      next = GetObjectProperty(npp_, element, "nextElementSibling");
    }
    // If no more siblings, find an ancestor with a sibling.
    while (!next) {
      NPObject* parent = GetObjectProperty(npp_, element, "parentNode");
      if (!parent) {
        break;
      }
      NPN_ReleaseObject(element);
      element = parent;
      next = GetObjectProperty(npp_, element, "nextElementSibling");
    }
    // Continue.
    NPN_ReleaseObject(element);
    element = next;
  }
}

NpapiElement::NpapiElement(NPP npp, NPObject* element)
    : npp_(npp), element_(element) {
  NPN_RetainObject(element_);
}

NpapiElement::~NpapiElement() {
  NPN_ReleaseObject(element_);
}

pagespeed::DomDocument* NpapiElement::GetContentDocument() const {
  NpapiDocument* rval = NULL;
  NPObject* document = GetObjectProperty(npp_, element_, "contentDocument");
  if (document) {
    rval = new NpapiDocument(npp_, document);
    // GetObjectProperty called NPN_RetainObject, as did the NpapiDocument
    // constructor.  We need to do one NPN_ReleaseObject here to cancel the
    // retain from GetObjectProperty, and then the NpapiDocument destructor
    // will do the final release when the NpapiDocument object is deleted.
    NPN_ReleaseObject(document);
  }
  return rval;
}

std::string NpapiElement::GetTagName() const {
  return DemandStringProperty(npp_, element_, "tagName");
}

bool NpapiElement::GetAttributeByName(const std::string& name,
                                      std::string* attr_value) const {
  return GetStringProperty(npp_, element_, name.c_str(), attr_value);
}

}  // namespace

pagespeed::DomDocument* CreateDocument(NPP npp, NPObject* document) {
  return new NpapiDocument(npp, document);
}

}  // namespace pagespeed_chromium
