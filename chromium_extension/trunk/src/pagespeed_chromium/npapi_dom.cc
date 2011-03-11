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
#include <vector>

#include "base/basictypes.h"
#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "base/stl_util-inl.h"
#include "base/string_number_conversions.h"

extern NPNetscapeFuncs* npnfuncs;

namespace pagespeed_chromium {

namespace {

// Try to get a object-valued property with the given name from the given
// NPObject.  If successful, return the NPObject pointer; otherwise return
// NULL.  The caller is responsible for calling NPN_ReleaseObject on the result
// (if it is non-NULL) when finished with it.
NPObject* GetObjectProperty(NPP npp, NPObject* object, const char* name) {
  NPObject* rval = NULL;
  NPVariant result;
  if (npnfuncs->getproperty(npp, object, npnfuncs->getstringidentifier(name),
                            &result)) {
    if (NPVARIANT_IS_OBJECT(result)) {
      rval = NPVARIANT_TO_OBJECT(result);
      npnfuncs->retainobject(rval);
    }
    npnfuncs->releasevariantvalue(&result);
  }
  return rval;
}

// Try to get a integer-valued property with the given name from the given
// NPObject.  If successful, put the value into the output int and return true;
// otherwise, return false.
bool GetIntProperty(NPP npp, NPObject* object, const char* name, int* output) {
  bool rval = false;
  NPVariant result;
  if (npnfuncs->getproperty(npp, object, npnfuncs->getstringidentifier(name),
                            &result)) {
    if (NPVARIANT_IS_INT32(result)) {
      *output = static_cast<int>(NPVARIANT_TO_INT32(result));
      rval = true;
    }
    npnfuncs->releasevariantvalue(&result);
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
  if (npnfuncs->getproperty(npp, object, npnfuncs->getstringidentifier(name),
                            &result)) {
    if (NPVARIANT_IS_STRING(result)) {
      const NPString& result_NPString = NPVARIANT_TO_STRING(result);
      output->assign(result_NPString.UTF8Characters,
                     result_NPString.UTF8Length);
      rval = true;
    }
    npnfuncs->releasevariantvalue(&result);
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

  // There are several different rules that want to traverse the DOM, but NPAPI
  // calls are expensive, so we memoize the Traverse() method.  The first time
  // it is called, we use NPAPI calls to walk the actual DOM and construct a
  // list of NpapiElement objects.  The next time it is called, it simply uses
  // the already-constructed list of NpapiElement objects.  The lifetime of
  // these NpapiElement objects is the lifetime of the NpapiDocument object, so
  // everything will be saved until the engine finishes running.
  mutable std::vector<const pagespeed::DomElement*> memo_elements_;

  DISALLOW_COPY_AND_ASSIGN(NpapiDocument);
};

class ProxyDocument : public pagespeed::DomDocument {
 public:
  explicit ProxyDocument(const pagespeed::DomDocument* document)
      : doc_(document) { DCHECK(doc_); }
  virtual ~ProxyDocument() {}

  virtual std::string GetDocumentUrl() const { return doc_->GetDocumentUrl(); }
  virtual std::string GetBaseUrl() const { return doc_->GetBaseUrl(); }
  virtual void Traverse(pagespeed::DomElementVisitor* visitor) const {
    doc_->Traverse(visitor); }

 private:
  const pagespeed::DomDocument* doc_;

  DISALLOW_COPY_AND_ASSIGN(ProxyDocument);
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

  virtual Status GetActualWidth(int* out_width) const;
  virtual Status GetActualHeight(int* out_height) const;
  virtual Status HasWidthSpecified(bool* out_width_specified) const;
  virtual Status HasHeightSpecified(bool* out_height_specified) const;

 private:
  // Get the value of a CSS property, whether defined in an inline style or in
  // an external stylesheet.
  bool GetCssPropertyByName(const std::string& name,
                            std::string* property_value) const;

  const NPP npp_;
  NPObject* const element_;

  // The GetTagName() method gets called by pretty much every rule that walks
  // the DOM, so it's worth memoizing, just as we memoize the Traverse() method
  // in NpapiDocument.
  mutable std::string memo_tag_name_;
  // We also memoize GetContentDocument(), so that we can then memoize
  // recursive Traverse()-als.
  mutable scoped_ptr<NpapiDocument> memo_content_document_;
  mutable bool got_content_document_;
  // We could also memoize, say, GetAttributeByName(), but different rules tend
  // to read different attributes on different elements, so it's not really
  // worth it.  Memoizing the above methods is already a big win.

  DISALLOW_COPY_AND_ASSIGN(NpapiElement);
};

NpapiDocument::NpapiDocument(NPP npp, NPObject* document)
    : npp_(npp), document_(document) {
  npnfuncs->retainobject(document_);
}

NpapiDocument::~NpapiDocument() {
  npnfuncs->releaseobject(document_);
  STLDeleteContainerPointers(memo_elements_.begin(), memo_elements_.end());
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
  // If we've called Traverse() before, use the memoized element objects.
  if (!memo_elements_.empty()) {
    for (std::vector<const pagespeed::DomElement*>::const_iterator iter =
             memo_elements_.begin(), end = memo_elements_.end();
         iter != end; ++iter) {
      visitor->Visit(**iter);
    }
    return;
  }

  // We've never done a Traverse() before, so walk the DOM and memoize the
  // NpapiElement objects we create.
  NPObject* element = GetObjectProperty(npp_, document_, "documentElement");
  while (element) {
    // Visit the element.
    const NpapiElement* chromium_element = new NpapiElement(npp_, element);
    memo_elements_.push_back(chromium_element);
    visitor->Visit(*chromium_element);
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
      npnfuncs->releaseobject(element);
      element = parent;
      next = GetObjectProperty(npp_, element, "nextElementSibling");
    }
    // Continue.
    npnfuncs->releaseobject(element);
    element = next;
  }
}

NpapiElement::NpapiElement(NPP npp, NPObject* element)
    : npp_(npp), element_(element), got_content_document_(false) {
  npnfuncs->retainobject(element_);
}

NpapiElement::~NpapiElement() {
  npnfuncs->releaseobject(element_);
}

pagespeed::DomDocument* NpapiElement::GetContentDocument() const {
  if (!got_content_document_) {
    DCHECK(memo_content_document_.get() == NULL);
    NPObject* document = GetObjectProperty(npp_, element_, "contentDocument");
    if (document) {
      memo_content_document_.reset(new NpapiDocument(npp_, document));
      // GetObjectProperty called NPN_RetainObject, as did the NpapiDocument
      // constructor.  We need to do one NPN_ReleaseObject here to cancel the
      // retain from GetObjectProperty, and then the NpapiDocument destructor
      // will do the final release when the NpapiDocument object is deleted.
      npnfuncs->releaseobject(document);
    }
    got_content_document_ = true;
  }

  // We can't return memo_content_document_ directly, because the caller will
  // then free the object.  Instead, we return a proxy object, which the caller
  // may free.
  if (memo_content_document_.get() != NULL) {
    return new ProxyDocument(memo_content_document_.get());
  }
  return NULL;
}

std::string NpapiElement::GetTagName() const {
  if (memo_tag_name_.empty()) {
    memo_tag_name_ = DemandStringProperty(npp_, element_, "tagName");
  }
  return memo_tag_name_;
}

bool NpapiElement::GetAttributeByName(const std::string& name,
                                      std::string* attr_value) const {
  bool rval = false;
  NPVariant argument, result;
  STRINGN_TO_NPVARIANT(name.data(), name.size(), argument);
  // Note that we won't call NPN_ReleaseVariantValue on &argument, because that
  // would call NPN_MemFree on its data, but that data was allocated by a
  // std::string rather than by NPN_MemAlloc.
  if (npnfuncs->invoke(npp_, element_,
                       npnfuncs->getstringidentifier("getAttribute"),
                       &argument, 1, &result)) {
    if (NPVARIANT_IS_STRING(result)) {
      const NPString& result_NPString = NPVARIANT_TO_STRING(result);
      attr_value->assign(result_NPString.UTF8Characters,
                         result_NPString.UTF8Length);
      rval = true;
    }
    npnfuncs->releasevariantvalue(&result);
  }
  return rval;
}

bool NpapiElement::GetCssPropertyByName(const std::string& name,
                                        std::string* attr_value) const {
  return false;  // TODO(mdsteele): Figure out how to implement this.
}

NpapiElement::Status NpapiElement::GetActualWidth(int* out_width) const {
  return (GetIntProperty(npp_, element_, "width", out_width) ?
          SUCCESS : FAILURE);
}

NpapiElement::Status NpapiElement::GetActualHeight(int* out_height) const {
  return (GetIntProperty(npp_, element_, "height", out_height) ?
          SUCCESS : FAILURE);
}

NpapiElement::Status
NpapiElement::HasWidthSpecified(bool* out_width_specified) const {
  std::string value;
  *out_width_specified = (GetAttributeByName("width", &value) ||
                          (GetCssPropertyByName("width", &value) &&
                           !value.empty()));
  return SUCCESS;
}

NpapiElement::Status
NpapiElement::HasHeightSpecified(bool* out_height_specified) const {
  std::string value;
  *out_height_specified = (GetAttributeByName("height", &value) ||
                           (GetCssPropertyByName("height", &value) &&
                            !value.empty()));
  return SUCCESS;
}

}  // namespace

pagespeed::DomDocument* CreateDocument(NPP npp, NPObject* document) {
  return new NpapiDocument(npp, document);
}

}  // namespace pagespeed_chromium
