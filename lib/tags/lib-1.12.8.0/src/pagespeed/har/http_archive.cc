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

#include <ctype.h>  // for isdigit
#include <stdio.h>  // for sscanf

#include "base/basictypes.h"
#include "base/logging.h"
#include "base/json/json_reader.h"
#include "base/scoped_ptr.h"
#include "base/string_util.h"
#include "base/third_party/nspr/prtime.h"
#include "base/values.h"
#include "pagespeed/core/pagespeed_input.h"
#include "third_party/modp_b64/modp_b64.h"

namespace pagespeed {

namespace {

class InputPopulator {
 public:
  static bool Populate(const Value& har_json, PagespeedInput* input);

 private:
  enum HeaderType { REQUEST_HEADERS, RESPONSE_HEADERS };

  InputPopulator() : page_started_millis_(-1), error_(false) {}
  ~InputPopulator() {}

  void PopulateInput(const Value& har_json, PagespeedInput* input);
  void DeterminePageTimings(const DictionaryValue& log_json,
                            PagespeedInput* input);
  void PopulateResource(const DictionaryValue& entry_json, Resource* resource);
  void PopulateHeaders(const DictionaryValue& headers_json, HeaderType htype,
                       Resource* resource);
  std::string GetString(const DictionaryValue& object, const std::string& key);

  int64 page_started_millis_;
  bool error_;

  DISALLOW_COPY_AND_ASSIGN(InputPopulator);
};

bool InputPopulator::Populate(const Value& har_json, PagespeedInput* input) {
  InputPopulator populator;
  populator.PopulateInput(har_json, input);
  return !populator.error_;
}

// Macro to be used only within InputPopulator instance methods:
#define INPUT_POPULATOR_ERROR() error_ = true; LOG(ERROR)

void InputPopulator::PopulateInput(const Value& har_json,
                                   PagespeedInput* input) {
  if (!har_json.IsType(Value::TYPE_DICTIONARY)) {
    INPUT_POPULATOR_ERROR() << "Top-level JSON value must be an object.";
    return;
  }

  DictionaryValue* log_json;
  if (!static_cast<const DictionaryValue&>(har_json).
      GetDictionary("log", &log_json)) {
    INPUT_POPULATOR_ERROR() << "\"log\" field must be an object.";
    return;
  }

  DeterminePageTimings(*log_json, input);

  ListValue* entries_json;
  if (!log_json->GetList("entries", &entries_json)) {
    INPUT_POPULATOR_ERROR() << "\"entries\" field must be an array.";
    return;
  }

  for (size_t index = 0, size = entries_json->GetSize();
       index < size; ++index) {
    DictionaryValue* entry_json;
    if (!entries_json->GetDictionary(index, &entry_json)) {
      INPUT_POPULATOR_ERROR() << "Entry item must be an object.";
      continue;
    }
    scoped_ptr<Resource> resource(new Resource);
    PopulateResource(*entry_json, resource.get());
    if (!error_) {
      input->AddResource(resource.release());
    }
  }
}

void InputPopulator::DeterminePageTimings(
    const DictionaryValue& log_json, PagespeedInput* input) {
  ListValue* pages_json;
  if (!log_json.GetList("pages", &pages_json)) {
    // The "pages" field is optional, so give up without error if it's not
    // there.
    return;
  }

  // For now, just take the first page (if any), and ignore others.
  // TODO(mdsteele): Behave intelligently in the face of multiple pages.
  if (pages_json->GetSize() < 1) {
    return;
  }
  DictionaryValue* page_json;
  if (!pages_json->GetDictionary(0, &page_json)) {
    INPUT_POPULATOR_ERROR() << "Page item must be an object.";
    return;
  }

  const std::string started_datetime(GetString(*page_json, "startedDateTime"));
  if (!Iso8601ToEpochMillis(started_datetime, &page_started_millis_)) {
    INPUT_POPULATOR_ERROR() << "Malformed pages.startedDateTime: "
                            << started_datetime;
  }

  double onload_millis;
  if (page_json->GetDouble("pageTimings.onLoad", &onload_millis)) {
    if (onload_millis < 0) {
      // When onLoad is specified but negative, it indicates that
      // onload has not yet fired.
      input->SetOnloadState(PagespeedInput::ONLOAD_NOT_YET_FIRED);
    } else {
      input->SetOnloadTimeMillis(static_cast<int>(onload_millis));
    }
  }
}

void InputPopulator::PopulateResource(const DictionaryValue& entry_json,
                                      Resource* resource) {
  // Determine if the resource was loaded after onload.
  {
    std::string started_datetime;
    if (entry_json.GetString("startedDateTime", &started_datetime)) {
      int64 started_millis;
      if (Iso8601ToEpochMillis(started_datetime, &started_millis)) {
        if (page_started_millis_ > 0) {
          int64 request_start_time_millis =
              started_millis - page_started_millis_;
          // Truncate to 32 bits, which gives us a range of about 24
          // days.
          if (request_start_time_millis > kint32max) {
            LOG(INFO) << "Request starts more than kint32max milliseconds "
                      << "in the future. Truncating.";
            request_start_time_millis = kint32max;
          }
          // Don't SetRequestStartTimeMillis if request_start_time_millis is
          // negative, as that will result in an error down the line.
          if (request_start_time_millis < 0) {
            LOG(WARNING) << "Request starts before page starts.";
          } else {
            resource->SetRequestStartTimeMillis(
                static_cast<int>(request_start_time_millis));
          }
        }
      } else {
        INPUT_POPULATOR_ERROR() << "Malformed resource startedDateTime: "
                                << started_datetime;
      }
    }
  }

  // Get the request information.
  {
    DictionaryValue* request_json;
    if (!entry_json.GetDictionary("request", &request_json)) {
      INPUT_POPULATOR_ERROR() << "\"request\" field must be an object.";
      return;
    }

    resource->SetRequestMethod(GetString(*request_json, "method"));
    resource->SetRequestUrl(GetString(*request_json, "url"));
    PopulateHeaders(*request_json, REQUEST_HEADERS, resource);

    // Check for optional post data.
    std::string post_data;
    if (request_json->GetString("postData.text", &post_data)) {
      resource->SetRequestBody(post_data);
    }
  }

  // Get the response information.
  {
    DictionaryValue* response_json;
    if (!entry_json.GetDictionary("response", &response_json)) {
      INPUT_POPULATOR_ERROR() << "\"response\" field must be an object.";
      return;
    }

    { // Get the response HTTP version, if it's available.
      std::string protocol;
      if (response_json->GetString("httpVersion", &protocol)) {
        resource->SetResponseProtocol(protocol);
      }
    }

    { // Get the response status code, if it's available.
      int status;
      if (response_json->GetInteger("status", &status)) {
        resource->SetResponseStatusCode(status);
      }
    }

    PopulateHeaders(*response_json, RESPONSE_HEADERS, resource);

    DictionaryValue* content_json;
    if (!response_json->GetDictionary("content", &content_json)) {
      INPUT_POPULATOR_ERROR() << "\"content\" field must be an object.";
      return;
    }

    std::string content_text;
    if (content_json->GetString("text", &content_text)) {
      std::string encoding;
      if (!content_json->GetString("encoding", &encoding) || encoding == "") {
        resource->SetResponseBody(content_text);
      } else if (encoding == "base64") {
        std::string decoded_body;
        // Reserve enough space to decode into.
        decoded_body.resize(modp_b64_decode_len(content_text.size()));
        // Decode into the string's buffer.
        const int decoded_size = modp_b64_decode(&(decoded_body[0]),
                                                 content_text.data(),
                                                 content_text.size());
        if (decoded_size >= 0) {
          // Resize the buffer to the actual decoded size.
          decoded_body.resize(decoded_size);
          resource->SetResponseBody(decoded_body);
        } else {
          INPUT_POPULATOR_ERROR() << "Failed to base64-decode response content.";
        }
      } else {
        INPUT_POPULATOR_ERROR() << "Received unexpected encoding: " << encoding;
      }
    }
  }
}

void InputPopulator::PopulateHeaders(const DictionaryValue& json,
                                     HeaderType htype,
                                     Resource* resource) {
  ListValue* headers_json;
  if (!json.GetList("headers", &headers_json)) {
    INPUT_POPULATOR_ERROR() << "\"headers\" field must be an array.";
    return;
  }

  for (size_t index = 0, size = headers_json->GetSize();
       index < size; ++index) {
    DictionaryValue* header_json;
    if (!headers_json->GetDictionary(index,  &header_json)) {
      INPUT_POPULATOR_ERROR() << "Header item must be an object.";
      continue;
    }

    const std::string name = GetString(*header_json, "name");
    const std::string value = GetString(*header_json, "value");

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

std::string InputPopulator::GetString(const DictionaryValue& object,
                                      const std::string& key) {
  std::string value;
  if (!object.GetString(key, &value)) {
    INPUT_POPULATOR_ERROR() << '"' << key << "\" field must be a string.";
  }
  return value;
}

}  // namespace

// NOTE: takes ownership of the filter instance.
PagespeedInput* ParseHttpArchiveWithFilter(const std::string& har_data,
                                           ResourceFilter* filter) {
  scoped_ptr<ResourceFilter> resource_filter(filter);
  std::string error_msg_out;
  scoped_ptr<const Value> har_json(base::JSONReader::ReadAndReturnError(
      har_data,
      true,  // allow_trailing_comma
      NULL,  // error_code_out (ReadAndReturnError permits NULL here)
      &error_msg_out));
  if (har_json == NULL) {
    LOG(ERROR) << "Failed to parse JSON: " << error_msg_out;
    return NULL;
  }

  scoped_ptr<PagespeedInput> input(
      resource_filter == NULL ?
      new PagespeedInput() :
      new PagespeedInput(resource_filter.release()));
  if (InputPopulator::Populate(*har_json, input.get())) {
    return input.release();
  } else {
    return NULL;
  }
}

PagespeedInput* ParseHttpArchive(const std::string& har_data) {
  return ParseHttpArchiveWithFilter(har_data, NULL);
}

// TODO(mdsteele): It would be nice to have a more robust ISO 8601 parser here,
// but this one seems to do okay for now on our unit tests.
bool Iso8601ToEpochMillis(const std::string& input, int64* output) {
  // We need to use unsigned ints, because otherwise sscanf() will look for +/-
  // characters, which we do not want to allow.
  unsigned int year, month, day, hours, minutes, seconds;
  char tail[21];  // The tail of the string, for milliseconds and timezone.
  // Parse the first six fields, and store the remainder of the string in tail.
  // Fail if we don't successfully parse all the fields.
  if (sscanf(input.c_str(), "%4u-%2u-%2uT%2u:%2u:%2u%20s",  // NOLINT
             &year, &month, &day, &hours, &minutes, &seconds, tail) != 7) {
    return false;
  }
  // Fail if any of the fields so far are obviously out of range.
  if (month < 1 || month > 12 || day < 1 || day > 31 ||
      hours > 23 || minutes > 59 || seconds > 59) {
    return false;
  }

  // Get the fractional part of the seconds, if any.  This is sort of ugly,
  // because we have to interpret ".3" as ".300" and not ".003", so we can't
  // just use sscanf.  Also, there may be more digits than we want
  // (e.g. ".123456"), so this gracefully ignores the extra digits.  Of course,
  // because tail is of static size, we'll still fail if there are dozens of
  // digits of precision.  Oh, well.
  int milliseconds = 0;
  int index = 0;  // The current index into tail.
  if (tail[0] == '.') {
    int multiplier = 100;
    index = 1;
    while (IsAsciiDigit(tail[index])) {
      milliseconds += (tail[index] - '0') * multiplier;
      multiplier /= 10;
      ++index;
    }
  }
  const PRInt32 microseconds = PR_USEC_PER_MSEC * milliseconds;

  // Now, index is pointing at the beginning of the timezone spec.  The
  // timezone should be "Z" for UTC, or something like e.g. "-05:00" for EST.
  int tz_offset_seconds = 0;
  DCHECK(index < static_cast<int>(arraysize(tail)));
  const char tz_sign = tail[index];
  if (tz_sign == 'Z') {
    // We're dealing with UTC.  Fail if the "Z" is not the last character of
    // the string.
    ++index;
    DCHECK(index < static_cast<int>(arraysize(tail)));
    if (tail[index] != '\0') {
      return false;
    }
  } else if (tz_sign == '+' || tz_sign == '-') {
    // We have a timezone offset.  Use sscanf to get the hours and minutes of
    // the offset.  The 'ignored' char is a hack to make sure that this is the
    // end of the string -- if sscanf doesn't parse exactly 2 out of the 3
    // format arguments, then we fail.
    unsigned int tz_hours, tz_minutes;
    char ignored;
    if (sscanf(tail + index + 1, "%2u:%2u%c",  // NOLINT
               &tz_hours, &tz_minutes, &ignored) != 2) {
      return false;
    }
    tz_offset_seconds = ((tz_hours * 3600 + tz_minutes * 60) *
                         (tz_sign == '+' ? 1 : -1));
  } else {
    // The timezone is invalid, so fail.
    return false;
  }

  // Finally, use the prtime library to calculate the milliseconds since the
  // epoch UTC for this datetime.
  PRExplodedTime exploded = {
    microseconds,
    static_cast<PRInt32>(seconds),
    static_cast<PRInt32>(minutes),
    static_cast<PRInt32>(hours),
    static_cast<PRInt32>(day),
    static_cast<PRInt32>(month - 1),
    static_cast<PRInt16>(year),
    0,
    0,
    { static_cast<PRInt32>(tz_offset_seconds), 0 }
  };
  // We need to explicitly cast PR_USEC_PER_MSEC to a signed type here;
  // otherwise, we will perform unsigned division, yielding a wrong result for
  // "negative" datetimes (i.e. those before 1970).
  *output = PR_ImplodeTime(&exploded) / static_cast<int64>(PR_USEC_PER_MSEC);
  return true;
}

}  // namespace pagespeed
