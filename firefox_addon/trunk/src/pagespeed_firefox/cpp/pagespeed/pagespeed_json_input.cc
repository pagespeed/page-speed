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

#include "base/basictypes.h"
#include "base/logging.h"
#include "pagespeed/core/javascript_call_info.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "third_party/cJSON/cJSON.h"

namespace pagespeed {

namespace {

// These next two classes aid with the PopulateHeaders template method in the
// InputPopulator class.

class AddRequestHeader {
 public:
  explicit AddRequestHeader(Resource* resource) : resource_(resource) {}
  ~AddRequestHeader() {}
  void operator() (const std::string &key, const std::string &value) {
    resource_->AddRequestHeader(key, value);
  }
 private:
  Resource *resource_;
  DISALLOW_COPY_AND_ASSIGN(AddRequestHeader);
};

class AddResponseHeader {
 public:
  explicit AddResponseHeader(Resource* resource) : resource_(resource) {}
  ~AddResponseHeader() {}
  void operator() (const std::string &key, const std::string &value) {
    resource_->AddResponseHeader(key, value);
  }
 private:
  Resource *resource_;
  DISALLOW_COPY_AND_ASSIGN(AddResponseHeader);
};

// The InputPopulator class below allows us to populate a PagespeedInput object
// from JSON data, while keeping track of our error state.

class InputPopulator {
 public:
  // Parse the JSON string and use it to populate the input.  If any errors
  // occur, log them and return false, otherwise return true.
  static bool Populate(PagespeedInput *input, const char *json_data,
                       const std::vector<std::string> &contents);

 private:
  explicit InputPopulator(const std::vector<std::string> *contents)
      : contents_(contents), error_(false) {}
  ~InputPopulator() {}

  // Extract an integer from a JSON value.
  int ToInt(cJSON *value);

  // Extract a string from a JSON value.
  const std::string ToString(cJSON *value);

  // Get the contents of the body to which the JSON value refers.
  const std::string RetrieveBody(cJSON *attribute_json);

  // Given a means of adding headers to a resource, and JSON value representing
  // a list of headers, add the headers represented by the JSON.
  template <class AddHeader>
  void PopulateHeaders(AddHeader &add_header, cJSON *attribute_json);

  // Given a JSON value representing all JavaScript calls, add those
  // calls to the Resource object.
  void PopulateJsCalls(Resource* resource, cJSON *attribute_json);

  // Given a JSON value representing one attribute of a resource, set the
  // corresponding attribute on the Resource object.
  void PopulateAttribute(Resource *resource, cJSON *attribute_json);

  // Given a JSON value representing a single resource, populate the Resource
  // object.
  void PopulateResource(Resource *resource, cJSON *resource_json);

  // Given a JSON value representing a list of resources, populate the
  // PagespeedInput object.
  void PopulateInput(PagespeedInput *input, cJSON *resources_json);

  bool error_;  // true if there's been at least one error, false otherwise
  const std::vector<std::string> *contents_;

  DISALLOW_COPY_AND_ASSIGN(InputPopulator);
};

// Macro to be used only within InputPopulator instance methods:
#define INPUT_POPULATOR_ERROR() ((error_ = true), LOG(DFATAL))

int InputPopulator::ToInt(cJSON *value) {
  if (value->type == cJSON_Number) {
    return value->valueint;
  } else {
    INPUT_POPULATOR_ERROR() << "Expected integer value.";
    return 0;
  }
}

const std::string InputPopulator::ToString(cJSON *value) {
  if (value->type == cJSON_String) {
    return value->valuestring;
  } else {
    INPUT_POPULATOR_ERROR() << "Expected string value.";
    return "";
  }
}

const std::string InputPopulator::RetrieveBody(cJSON *attribute_json) {
  const int index = ToInt(attribute_json);
  if (0 <= index && index < contents_->size()) {
    return contents_->at(index);
  } else {
    INPUT_POPULATOR_ERROR() << "Body index out of range: " << index;
    return "";
  }
}

template <class AddHeader>
void InputPopulator::PopulateHeaders(AddHeader &add_header,
                                     cJSON *attribute_json) {
  if (attribute_json->type != cJSON_Array) {
    INPUT_POPULATOR_ERROR() << "Expected array value for key: "
                            << attribute_json->string;
    return;
  }

  for (cJSON *header_json = attribute_json->child;
       header_json != NULL; header_json = header_json->next) {
    if (header_json->type != cJSON_Array) {
      INPUT_POPULATOR_ERROR() << "Expected array value for header entry.";
      continue;
    }

    if (cJSON_GetArraySize(header_json) != 2) {
      INPUT_POPULATOR_ERROR() << "Expected array of size 2 for header entry.";
      continue;
    }

    add_header(ToString(cJSON_GetArrayItem(header_json, 0)),
               ToString(cJSON_GetArrayItem(header_json, 1)));
  }
}

void InputPopulator::PopulateJsCalls(Resource* resource,
                                     cJSON *calls_json) {
  if (calls_json->type != cJSON_Array) {
    INPUT_POPULATOR_ERROR() << "Expected array value for key: "
                            << calls_json->string;
    return;
  }

  for (cJSON *call_json = calls_json->child;
       call_json != NULL; call_json = call_json->next) {
    if (call_json->type != cJSON_Object) {
      INPUT_POPULATOR_ERROR() << "Expected object value for js call entry.";
      continue;
    }

    // Extract the 'fn', 'args', 'line_number', and 'doc_url'
    // attributes for each entry.
    std::string fn;
    std::vector<std::string> args;
    int line_number = -1;
    std::string doc_url;
    for (cJSON *call_attribute_json = call_json->child;
         call_attribute_json != NULL;
         call_attribute_json = call_attribute_json->next) {
      const std::string& key = call_attribute_json->string;
      if (key == "fn") {
        fn = ToString(call_attribute_json);
      } else if (key == "args") {
        if (call_attribute_json->type != cJSON_Array) {
          INPUT_POPULATOR_ERROR() << "Expected array value for args.";
          return;
        }
        for (cJSON *arg_json = call_attribute_json->child;
             arg_json != NULL; arg_json = arg_json->next) {
          args.push_back(ToString(arg_json));
        }
      } else if (key == "line_number") {
        line_number = ToInt(call_attribute_json);
      } else if (key == "doc_url") {
        doc_url = ToString(call_attribute_json);
      } else {
        INPUT_POPULATOR_ERROR() << "Unexpected call attribute " << key;
        return;
      }
    }

    if (fn.length() > 0 &&
        args.size() > 0 &&
        line_number > 0 &&
        doc_url.length() > 0) {
      resource->AddJavaScriptCall(
          new JavaScriptCallInfo(fn, doc_url, args, line_number));
    }
    else {
      INPUT_POPULATOR_ERROR() << "Failed to populate JavaScriptCallInfo.";
      return;
    }
  }
}

void InputPopulator::PopulateAttribute(Resource *resource,
                                       cJSON *attribute_json) {
  const std::string &key = attribute_json->string;
  if (key == "req_url") {
    resource->SetRequestUrl(ToString(attribute_json));
  } else if (key == "req_method") {
    resource->SetRequestMethod(ToString(attribute_json));
  } else if (key == "req_headers") {
    AddRequestHeader add_header(resource);
    PopulateHeaders(add_header, attribute_json);
  } else if (key == "req_body") {
    resource->SetRequestBody(RetrieveBody(attribute_json));
  } else if (key == "req_cookies") {
    resource->SetCookies(ToString(attribute_json));
  } else if (key == "res_status") {
    resource->SetResponseStatusCode(ToInt(attribute_json));
  } else if (key == "res_headers") {
    AddResponseHeader add_header(resource);
    PopulateHeaders(add_header, attribute_json);
  } else if (key == "res_body") {
    resource->SetResponseBody(RetrieveBody(attribute_json));
  } else if (key == "req_lazy_loaded") {
    if (attribute_json->type == cJSON_True) {
      resource->SetLazyLoaded();
    } else if (attribute_json->type == cJSON_False) {
      // do nothing, resource defaults to not lazy-loaded.
    } else {
      INPUT_POPULATOR_ERROR() << "lazy_loaded should be true(1) or false(0). "
                              << key << ": " << attribute_json->type;
    }

  } else if (key == "js_calls") {
    PopulateJsCalls(resource, attribute_json);
  } else {
    INPUT_POPULATOR_ERROR() << "Unknown attribute key: " << key;
  }
}

void InputPopulator::PopulateResource(Resource *resource,
                                      cJSON *resource_json) {
  if (resource_json->type != cJSON_Object) {
    INPUT_POPULATOR_ERROR() << "Resource JSON value must be an object.";
    return;
  }

  for (cJSON *attribute_json = resource_json->child;
       attribute_json != NULL; attribute_json = attribute_json->next) {
    PopulateAttribute(resource, attribute_json);
  }
}

void InputPopulator::PopulateInput(PagespeedInput *input,
                                   cJSON *resources_json) {
  if (resources_json->type != cJSON_Array) {
    INPUT_POPULATOR_ERROR() << "Top-level JSON value must be an array.";
    return;
  }

  for (cJSON *resource_json = resources_json->child;
       resource_json != NULL; resource_json = resource_json->next) {
    Resource *resource = new Resource();
    PopulateResource(resource, resource_json);
    input->AddResource(resource);  // Ownership is transferred to input.
  }
}

bool InputPopulator::Populate(PagespeedInput *input, const char *json_data,
                              const std::vector<std::string> &contents) {
  bool ok = true;
  InputPopulator populator(&contents);
  cJSON *resources_json = cJSON_Parse(json_data);

  if (resources_json != NULL) {
    populator.PopulateInput(input, resources_json);
    ok = !populator.error_;
  } else {
    LOG(DFATAL) << "Input was not valid JSON.";
    ok = false;
  }

  cJSON_Delete(resources_json);
  return ok;
}

}  // namespace

bool PopulateInputFromJSON(PagespeedInput *input, const char *json_data,
                           const std::vector<std::string> &contents) {
  return InputPopulator::Populate(input, json_data, contents);
}

}  // namespace pagespeed
