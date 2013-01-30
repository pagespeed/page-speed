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

// Author: Matthew Steele

#include "pagespeed_firefox/cpp/pagespeed/pagespeed_json_input.h"

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/values.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"

namespace pagespeed {

namespace {

// The InputPopulator class below allows us to populate a PagespeedInput object
// from JSON data, while keeping track of our error state.
class InputPopulator {
 public:
  // Parse the JSON string and use it to populate the input.  If any errors
  // occur, log them and return false, otherwise return true.
  static bool Populate(PagespeedInput* input, const char* json_data);

 private:
  InputPopulator() : error_(false) {}
  ~InputPopulator() {}

  // Extract an integer from a JSON value.
  int ToInt(const Value& value);

  // Extract a string from a JSON value.
  const std::string ToString(const Value& value);

  // Given a JSON value representing one attribute of a resource, set the
  // corresponding attribute on the Resource object.
  void PopulateAttribute(const std::string& key,
                         const Value& attribute_json,
                         Resource* resource);

  // Given a JSON value representing a single resource, populate the Resource
  // object.
  void PopulateResource(const DictionaryValue& attribute_json,
                        Resource* resource);

  // Given a JSON value representing a list of resources, populate the
  // PagespeedInput object.
  void PopulateInput(const Value& attribute_json, PagespeedInput* input);

  bool error_;  // true if there's been at least one error, false otherwise

  DISALLOW_COPY_AND_ASSIGN(InputPopulator);
};

// Macro to be used only within InputPopulator instance methods:
#define INPUT_POPULATOR_ERROR() error_ = true; LOG(DFATAL)

int InputPopulator::ToInt(const Value& value) {
  int integer;
  if (value.GetAsInteger(&integer)) {
    return integer;
  } else {
    INPUT_POPULATOR_ERROR() << "Expected integer value.";
    return 0;
  }
}

const std::string InputPopulator::ToString(const Value& value) {
  std::string str;
  if (value.GetAsString(&str)) {
    return str;
  } else {
    INPUT_POPULATOR_ERROR() << "Expected string value.";
    return "";
  }
}

void InputPopulator::PopulateAttribute(const std::string& key,
                                       const Value& attribute_json,
                                       Resource *resource) {
  if (key == "url") {
    // Nothing to do. we already validated this field in
    // PopulateResource.
  } else if (key == "cookieString") {
    resource->SetCookies(ToString(attribute_json));
  } else {
    INPUT_POPULATOR_ERROR() << "Unknown attribute key: " << key;
  }
}

void InputPopulator::PopulateResource(const DictionaryValue& resource_json,
                                      Resource* resource) {
  for (DictionaryValue::key_iterator iter = resource_json.begin_keys(),
           end = resource_json.end_keys(); iter != end; ++iter) {
    Value* attribute_json;
    if (resource_json.Get(*iter, &attribute_json)) {
      PopulateAttribute(*iter, *attribute_json, resource);
    }
  }
}

void InputPopulator::PopulateInput(const Value& resources_json,
                                   PagespeedInput* input) {
  if (!resources_json.IsType(Value::TYPE_LIST)) {
    INPUT_POPULATOR_ERROR() << "Top-level JSON value must be an array.";
    return;
  }
  const ListValue& list_json = *static_cast<const ListValue*>(&resources_json);

  for (size_t index = 0, size = list_json.GetSize(); index < size; ++index) {
    DictionaryValue* resource_json;
    if (!list_json.GetDictionary(index, &resource_json)) {
      INPUT_POPULATOR_ERROR() << "Resource JSON value must be an object";
      continue;
    }

    std::string url;
    if (!resource_json->GetStringWithoutPathExpansion("url", &url)) {
      INPUT_POPULATOR_ERROR() << "\"url\" field must be a string";
      continue;
    }

    Resource* resource = input->GetMutableResourceWithUrlOrNull(url);
    if (resource == NULL) {
      // This can happen if a resource filter was applied.
      continue;
    }

    PopulateResource(*resource_json, resource);
  }
}

bool InputPopulator::Populate(PagespeedInput *input, const char *json_data) {
  std::string error_msg_out;
  scoped_ptr<const Value> resources_json(base::JSONReader::ReadAndReturnError(
      json_data,
      true,  // allow_trailing_comma
      NULL,  // error_code_out (ReadAndReturnError permits NULL here)
      &error_msg_out));

  if (resources_json == NULL) {
    LOG(DFATAL) << "Input was not valid JSON: " << error_msg_out;
    return false;
  }

  InputPopulator populator;
  populator.PopulateInput(*resources_json, input);
  return !populator.error_;
}

}  // namespace

bool PopulateInputFromJSON(PagespeedInput *input, const char *json_data) {
  return InputPopulator::Populate(input, json_data);
}

}  // namespace pagespeed
