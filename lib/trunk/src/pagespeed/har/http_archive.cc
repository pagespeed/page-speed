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

#include "pagespeed/har/http_archive.h"

#include "base/basictypes.h"
#include "base/logging.h"
#include "pagespeed/core/pagespeed_input.h"
#include "third_party/cJSON/cJSON.h"
#include "third_party/modp_b64/modp_b64.h"

namespace pagespeed {

namespace {

class InputPopulator {
 public:
  static bool Populate(cJSON* har_json, PagespeedInput* input);

 private:
  enum HeaderType { REQUEST_HEADERS, RESPONSE_HEADERS };

  InputPopulator() : error_(false) {}
  ~InputPopulator() {}

  void PopulateInput(cJSON* har_json, PagespeedInput* input);
  void PopulateResource(cJSON* entry_json, Resource* resource);
  void PopulateHeaders(cJSON* headers_json, HeaderType htype,
                       Resource* resource);
  int GetInt(cJSON *object, const char* key);
  const std::string GetString(cJSON *object, const char* key);

  bool error_;

  DISALLOW_COPY_AND_ASSIGN(InputPopulator);
};

bool InputPopulator::Populate(cJSON* har_json, PagespeedInput* input) {
  InputPopulator populator;
  populator.PopulateInput(har_json, input);
  return !populator.error_;
}

// Macro to be used only within InputPopulator instance methods:
#define INPUT_POPULATOR_ERROR() ((error_ = true), LOG(ERROR))

void InputPopulator::PopulateInput(cJSON* har_json, PagespeedInput* input) {
  if (har_json->type != cJSON_Object) {
    INPUT_POPULATOR_ERROR() << "Top-level JSON value must be an object.";
    return;
  }

  cJSON* log_json = cJSON_GetObjectItem(har_json, "log");
  if (log_json == NULL || log_json->type != cJSON_Object) {
    INPUT_POPULATOR_ERROR() << "\"log\" field must be an object.";
    return;
  }

  // TODO: Check HAR version to make sure we're compatible.

  cJSON* entries_json = cJSON_GetObjectItem(log_json, "entries");
  if (entries_json == NULL || entries_json->type != cJSON_Array) {
    INPUT_POPULATOR_ERROR() << "\"entries\" field must be an array.";
    return;
  }

  for (cJSON* entry_json = entries_json->child;
       entry_json != NULL; entry_json = entry_json->next) {
    Resource* resource = new Resource;
    PopulateResource(entry_json, resource);
    if (error_) {
      delete resource;
    } else {
      input->AddResource(resource);
    }
  }
}

void InputPopulator::PopulateResource(cJSON* entry_json, Resource* resource) {
  if (entry_json->type != cJSON_Object) {
    INPUT_POPULATOR_ERROR() << "Entry item must be an object.";
    return;
  }

  // TODO: Use timings fields to determine if the resource was lazy-loaded.

  {
    cJSON* request_json = cJSON_GetObjectItem(entry_json, "request");
    if (request_json == NULL || request_json->type != cJSON_Object) {
      INPUT_POPULATOR_ERROR() << "\"request\" field must be an object.";
      return;
    }

    resource->SetRequestMethod(GetString(request_json, "method"));
    resource->SetRequestUrl(GetString(request_json, "url"));
    resource->SetRequestProtocol(GetString(request_json, "httpVersion"));
    PopulateHeaders(cJSON_GetObjectItem(request_json, "headers"),
                    REQUEST_HEADERS, resource);

    // Check for optional post data.
    cJSON* post_json = cJSON_GetObjectItem(request_json, "postData");
    if (post_json != NULL) {
      if (request_json->type != cJSON_Object) {
        INPUT_POPULATOR_ERROR() << "\"postData\" field must be an object.";
      } else {
        resource->SetRequestBody(GetString(post_json, "text"));
      }
    }
  }

  {
    cJSON* response_json = cJSON_GetObjectItem(entry_json, "response");
    if (response_json == NULL || response_json->type != cJSON_Object) {
      INPUT_POPULATOR_ERROR() << "\"response\" field must be an object.";
      return;
    }

    resource->SetResponseStatusCode(GetInt(response_json, "status"));
    resource->SetResponseProtocol(GetString(response_json, "httpVersion"));
    PopulateHeaders(cJSON_GetObjectItem(response_json, "headers"),
                    RESPONSE_HEADERS, resource);

    cJSON* content_json = cJSON_GetObjectItem(response_json, "content");
    if (response_json == NULL || response_json->type != cJSON_Object) {
      INPUT_POPULATOR_ERROR() << "\"content\" field must be an object.";
    } else {
      cJSON* content_text_json = cJSON_GetObjectItem(content_json, "text");
      if (content_text_json != NULL) {
        if (content_text_json->type != cJSON_String) {
          INPUT_POPULATOR_ERROR() << "\"text\" field must be a string.";
        } else {
          cJSON* content_text_encoding =
              cJSON_GetObjectItem(content_json, "encoding");
          if (content_text_encoding != NULL) {
            if (content_text_encoding->type != cJSON_String) {
              INPUT_POPULATOR_ERROR() << "\"encoding\" field must be a string.";
            } else {
              if (strcmp(content_text_encoding->valuestring, "base64") != 0) {
                INPUT_POPULATOR_ERROR() << "Received unexpected encoding: "
                                        << content_text_encoding->valuestring;
              } else {
                const char* encoded_body = content_text_json->valuestring;
                const size_t encoded_body_len = strlen(encoded_body);
                std::string decoded_body;

                // Reserve enough space to decode into.
                decoded_body.resize(modp_b64_decode_len(encoded_body_len));

                // Decode into the string's buffer.
                int decoded_size = modp_b64_decode(&(decoded_body[0]),
                                                   encoded_body,
                                                   encoded_body_len);

                if (decoded_size >= 0) {
                  // Resize the buffer to the actual decoded size.
                  decoded_body.resize(decoded_size);
                  resource->SetResponseBody(decoded_body);
                } else {
                  INPUT_POPULATOR_ERROR()
                      << "Failed to base64-decode response content.";
                }
              }
            }
          } else {
            resource->SetResponseBody(content_text_json->valuestring);
          }
        }
      }
    }
  }
}

void InputPopulator::PopulateHeaders(cJSON* headers_json, HeaderType htype,
                                     Resource* resource) {
  if (headers_json == NULL || headers_json->type != cJSON_Array) {
    INPUT_POPULATOR_ERROR() << "\"headers\" field must be an array.";
    return;
  }

  for (cJSON* header_json = headers_json->child;
       header_json != NULL; header_json = header_json->next) {
    if (header_json->type != cJSON_Object) {
      INPUT_POPULATOR_ERROR() << "Header item must be an object.";
      continue;
    }

    const std::string name = GetString(header_json, "name");
    const std::string value = GetString(header_json, "value");

    switch (htype) {
      case REQUEST_HEADERS:
        resource->AddRequestHeader(name, value);
        break;
      case RESPONSE_HEADERS:
        resource->AddResponseHeader(name, value);
        break;
      default:
        DCHECK(false);
    }
  }
}

int InputPopulator::GetInt(cJSON* object, const char* key) {
  DCHECK(object != NULL && object->type == cJSON_Object);
  cJSON* value = cJSON_GetObjectItem(object, key);
  if (value != NULL && value->type == cJSON_Number) {
    return value->valueint;
  } else {
    INPUT_POPULATOR_ERROR() << '"' << key << "\" field must be a number.";
    return 0;
  }
}

const std::string InputPopulator::GetString(cJSON* object, const char* key) {
  DCHECK(object != NULL && object->type == cJSON_Object);
  cJSON* value = cJSON_GetObjectItem(object, key);
  if (value != NULL && value->type == cJSON_String) {
    return value->valuestring;
  } else {
    INPUT_POPULATOR_ERROR() << '"' << key << "\" field must be a string.";
    return "";
  }
}

}  // namespace

PagespeedInput* ParseHttpArchive(const std::string& har_data) {
  cJSON* har_json = cJSON_Parse(har_data.c_str());
  if (har_json == NULL) {
    return NULL;
  }

  PagespeedInput* input = new PagespeedInput;
  const bool ok = InputPopulator::Populate(har_json, input);

  cJSON_Delete(har_json);
  if (ok) {
    return input;
  } else {
    delete input;
    return NULL;
  }
}

}  // namespace pagespeed
