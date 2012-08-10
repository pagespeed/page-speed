// Copyright 2011 Google Inc.
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

#include "pagespeed/dom/json_dom.h"

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "base/values.h"

namespace pagespeed {

namespace dom {

namespace {

std::string DemandString(const DictionaryValue& dict, const std::string& key) {
  std::string out;
  if (!dict.GetStringWithoutPathExpansion(key, &out)) {
    LOG(DFATAL) << "Could not get string: " << key;
  }
  return out;
}

std::vector<int> DemandIntegerList(const DictionaryValue& dict,
                                   const std::string& key) {
  std::vector<int> out;
  ListValue* list;
  if (dict.GetListWithoutPathExpansion(key, &list)) {
    for (size_t idx = 0, size = list->GetSize(); idx < size; ++idx) {
      int num;
      list->GetInteger(idx, &num);
      out.push_back(num);
    }
  } else {
    // The key does not exist, keep the output empty.
  }

  return out;
}


class JsonDocument : public pagespeed::DomDocument {
 public:
  explicit JsonDocument(const DictionaryValue* json) { json_.reset(json); }
  virtual ~JsonDocument() {}

  // DomDocument interface:
  virtual std::string GetDocumentUrl() const;
  virtual std::string GetBaseUrl() const;
  virtual void Traverse(pagespeed::DomElementVisitor* visitor) const;

  bool GetElement(int index, DictionaryValue** element) const;

 private:
  scoped_ptr<const DictionaryValue> json_;

  DISALLOW_COPY_AND_ASSIGN(JsonDocument);
};

class JsonElement : public pagespeed::DomElement {
 public:
  JsonElement(const DictionaryValue* json, const JsonDocument* json_doc)
      : json_(json), json_doc_(json_doc) {}
  virtual ~JsonElement() {}

  // DomElement interface:
  virtual pagespeed::DomDocument* GetContentDocument() const;
  virtual std::string GetTagName() const;
  virtual bool GetAttributeByName(const std::string& name,
                                  std::string* attr_value) const;
  virtual Status HasWidthSpecified(bool* out_width_specified) const;
  virtual Status HasHeightSpecified(bool* out_height_specified) const;
  virtual Status GetActualWidth(int* out_width) const;
  virtual Status GetActualHeight(int* out_height) const;
  virtual Status GetNumChildren(size_t* number) const;
  virtual Status GetChild(const DomElement** child, size_t index) const;
 private:
  const DictionaryValue* json_;
  const JsonDocument* json_doc_;

  DISALLOW_COPY_AND_ASSIGN(JsonElement);
};

std::string JsonDocument::GetDocumentUrl() const {
  return DemandString(*json_, "documentUrl");
}

std::string JsonDocument::GetBaseUrl() const {
  return DemandString(*json_, "baseUrl");
}

void JsonDocument::Traverse(pagespeed::DomElementVisitor* visitor) const {
  ListValue* elements;
  if (!json_->GetListWithoutPathExpansion("elements", &elements)) {
    LOG(ERROR) << "missing \"elements\" in JSON for JsonDocument";
    return;
  }

  for (int index = 0, size = elements->GetSize(); index < size; ++index) {
    DictionaryValue* dict;
    if (!elements->GetDictionary(index, &dict)) {
      LOG(ERROR) << "non-object item in \"elements\" list";
      continue;
    }
    JsonElement element(dict, this);
    visitor->Visit(element);
  }
}

bool JsonDocument::GetElement(int index, DictionaryValue** element) const {
  ListValue* elements;
  if (!json_->GetListWithoutPathExpansion("elements", &elements)) {
    LOG(ERROR) << "missing \"elements\" in JSON for JsonDocument";
    return false;
  }

  if (!elements->GetDictionary(index, element)) {
    LOG(ERROR) << "non-object item in \"elements\" list";
    return false;
  }
  return true;
}

pagespeed::DomDocument* JsonElement::GetContentDocument() const {
  DictionaryValue* out;
  // TODO(lsong): Use ref counted instead of DeepCopy().
  return (json_->GetDictionaryWithoutPathExpansion("contentDocument", &out) ?
          new JsonDocument(static_cast<DictionaryValue*>(out->DeepCopy()))
          : NULL);
}

std::string JsonElement::GetTagName() const {
  return DemandString(*json_, "tag");
}

bool JsonElement::GetAttributeByName(const std::string& name,
                                     std::string* attr_value) const {
  return json_->GetString("attrs." + name, attr_value);
}

JsonElement::Status
JsonElement::HasWidthSpecified(bool* out_width_specified) const {
  // TODO(mdsteele): Also detect if width is specified in CSS.
  std::string value;
  *out_width_specified = (GetAttributeByName("width", &value) &&
                          !value.empty());
  return SUCCESS;
}

JsonElement::Status
JsonElement::HasHeightSpecified(bool* out_height_specified) const {
  // TODO(mdsteele): Also detect if height is specified in CSS.
  std::string value;
  *out_height_specified = (GetAttributeByName("height", &value) &&
                           !value.empty());
  return SUCCESS;
}

JsonElement::Status JsonElement::GetActualWidth(int* out_width) const {
  return (json_->GetIntegerWithoutPathExpansion("width", out_width) ?
          SUCCESS : FAILURE);
}

JsonElement::Status JsonElement::GetActualHeight(int* out_height) const {
  return (json_->GetIntegerWithoutPathExpansion("height", out_height) ?
          SUCCESS : FAILURE);
}

DomElement::Status JsonElement::GetNumChildren(size_t* number) const {
  *number = 0;
  ListValue* list;
  if (json_->GetListWithoutPathExpansion("children", &list)) {
    *number = list->GetSize();
  }
  return SUCCESS;
}

DomElement::Status JsonElement::GetChild(
    const DomElement** child, size_t index) const {
  std::vector<int> childrenIndices =
      DemandIntegerList(*json_, "children");
  if (index < 0 || index >= childrenIndices.size()) {
    child = NULL;
  } else {
    DictionaryValue* dict;
    json_doc_->GetElement(childrenIndices[index], &dict);
    *child = new JsonElement(dict, json_doc_);
  }
  return SUCCESS;
}
}  // namespace

pagespeed::DomDocument* CreateDocument(const DictionaryValue* json) {
  return new JsonDocument(json);
}

}  // namespace dom
}  // namespace pagespeed
