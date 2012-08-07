// Copyright 2012 Google Inc. All Rights Reserved.
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

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/scoped_ptr.h"
#include "base/values.h"
#include "pagespeed_chromium/pagespeed_chromium.h"
#include "pagespeed/testing/pagespeed_test.h"

namespace {

class PagespeedChromiumTest : public ::pagespeed_testing::PagespeedTest {};

const char* kBasicId = "id";

const char* kBasicHar =
    "{"
    "  \"log\":{"
    "    \"version\":\"1.2\","
    "    \"creator\":{\"name\":\"http_archive_test\", \"version\":\"1.0\"},"
    "    \"pages\":["
    "      {"
    "        \"startedDateTime\": \"2009-04-16T12:07:23.321Z\","
    "        \"id\": \"page_0\","
    "        \"title\": \"Example Page\","
    "        \"pageTimings\": {"
    "          \"onLoad\": 1500"
    "        }"
    "      }"
    "    ],"
    "    \"entries\":["
    "      {"
    "        \"pageref\": \"page_0\","
    "        \"startedDateTime\": \"2009-04-16T12:07:23.596Z\","
    "        \"request\":{"
    "          \"method\":\"GET\","
    "          \"url\":\"http://www.example.com/index.html\","
    "          \"httpVersion\":\"HTTP/1.1\","
    "          \"cookies\":[],"
    "          \"headers\":["
    "            {\"name\":\"X-Foo\", \"value\":\"bar\"}"
    "          ],"
    "          \"headersSize\":-1,"
    "          \"bodySize\":0"
    "        },"
    "        \"response\":{"
    "          \"status\":200,"
    "          \"statusText\":\"OK\","
    "          \"httpVersion\":\"HTTP/1.1\","
    "          \"cookies\":[],"
    "          \"headers\":["
    "            {\"name\":\"Content-Type\", \"value\":\"text/html\"}"
    "          ],"
    "          \"content\":{"
    "            \"size\":13,"
    "            \"mimeType\":\"text/html\","
    "            \"encoding\":\"\","
    "            \"text\":\"Hello, world!\""
    "          },"
    "          \"redirectUrl\":\"\","
    "          \"headersSize\":-1,"
    "          \"bodySize\":13"
    "        }"
    "      },"
    "      {"
    "        \"pageref\": \"page_0\","
    "        \"startedDateTime\": \"2009-05-16T12:07:25.596Z\","
    "        \"request\":{"
    "          \"method\":\"GET\","
    "          \"url\":\"http://www.example.com/postonload.js\","
    "          \"httpVersion\":\"HTTP/1.1\","
    "          \"cookies\":[],"
    "          \"headers\":[],"
    "          \"headersSize\":-1,"
    "          \"bodySize\":0"
    "        },"
    "        \"response\":{"
    "          \"status\":200,"
    "          \"statusText\":\"OK\","
    "          \"httpVersion\":\"HTTP/1.1\","
    "          \"cookies\":[],"
    "          \"headers\":["
    "            {\"name\":\"Content-Type\","
    "             \"value\":\"application/javascript\"}"
    "          ],"
    "          \"content\":{"
    "            \"size\":13,"
    "            \"mimeType\":\"application/javascript\","
    "            \"text\":\"Hello, world!\""
    "          },"
    "          \"redirectUrl\":\"\","
    "          \"headersSize\":-1,"
    "          \"bodySize\":13"
    "        }"
    "      }"
    "    ]"
    "  }"
    "}";

const char* kBasicDocument =
    "{\"documentUrl\":\"http://www.example.com/index.html\","
    " \"baseUrl\":\"http://www.example.com/\",\"elements\":["
    "  {\"tag\":\"H1\"},"
    "  {\"tag\":\"IFRAME\", \"contentDocument\":"
    "    {\"documentUrl\":\"foo.html\",\"baseUrl\":\"\",\"elements\":["
    "      {\"tag\":\"IMG\"},"
    "      {\"tag\":\"IFRAME\", \"contentDocument\":"
    "        {\"documentUrl\":\"bar.html\",\"baseUrl\":\"\",\"elements\":["
    "          {\"tag\":\"DIV\"}"
    "       ]}}"
    "    ]}},"
    "  {\"tag\":\"H2\"},"
    "  {\"tag\":\"IFRAME\", \"contentDocument\":"
    "    {\"documentUrl\":\"baz.html\", \"baseUrl\":\"\", \"elements\":["
    "      {\"tag\":\"P\"}"
    "    ]}},"
    "  {\"tag\":\"H3\"}"
    "]}";

const char* kBasicTimeline =
    "[{"
    "  \"startTime\":1305844473655.642,"
    "  \"endTime\":1305844473655.873,"
    "  \"type\":\"RecalculateStyles\","
    "  \"usedHeapSize\":3114208,"
    "  \"totalHeapSize\":5650432"
    "},{"
    "  \"startTime\":1305844473656.029,"
    "  \"data\":{"
    "    \"type\":\"mousedown\""
    "  },"
    "  \"endTime\":1305844473656.055,"
    "  \"type\":\"EventDispatch\","
    "  \"usedHeapSize\":3114208,"
    "  \"totalHeapSize\":5650432"
    "},{"
    "  \"type\":\"EvaluateScript\","
    "  \"data\":{"
    "    \"url\":\"http://example.com/reflow.html\","
    "    \"lineNumber\":4"
    "  },"
    "  \"children\":[{"
    "    \"type\":\"RecalculateStyles\","
    "    \"stackTrace\":[{"
    "      \"functionName\":\"triggerReflow\","
    "      \"url\":\"http://example.com/reflow.html\","
    "      \"lineNumber\":31,"
    "      \"columnNumber\":30"
    "    },{"
    "      \"functionName\":\"\","
    "      \"url\":\"http://example.com/reflow.html\","
    "      \"lineNumber\":1,"
    "      \"columnNumber\":1"
    "    }]"
    "  },{"
    "    \"type\":\"Layout\","
    "    \"stackTrace\":[{"
    "      \"functionName\":\"triggerReflow\","
    "      \"url\":\"http://example.com/reflow.html\","
    "      \"lineNumber\":31,"
    "      \"columnNumber\":30"
    "    },{"
    "      \"functionName\":\"\","
    "      \"url\":\"http://example.com/reflow.html\","
    "      \"lineNumber\":1,"
    "      \"columnNumber\":1"
    "    }],"
    "  }]"
    "}]";

const char* kFilterName = "all";
const char* kLocale = "en";

void AssertValidResponse(const std::string& out, const std::string& err) {
  ASSERT_TRUE(err.empty());
  std::string error_msg_out;
  scoped_ptr<const Value> response_json(base::JSONReader::ReadAndReturnError(
      out,
      true,  // allow_trailing_comma
      NULL,  // error_code_out (ReadAndReturnError permits NULL here)
      &error_msg_out));
  ASSERT_NE(static_cast<const Value*>(NULL), response_json.get());

  ASSERT_TRUE(response_json->IsType(Value::TYPE_DICTIONARY));
  const DictionaryValue* root =
      static_cast<const DictionaryValue*>(response_json.get());

  ListValue* results = NULL;
  ASSERT_TRUE(root->GetList("results.rule_results", &results));

  // As a basic test, check for the presence of a
  // SpecifyACacheValidator result.
  DictionaryValue* cache_validator_result = NULL;
  for (size_t i = 0; i < results->GetSize(); ++i) {
    DictionaryValue* result = NULL;
    ASSERT_TRUE(results->GetDictionary(i, &result));
    std::string rule_name;
    ASSERT_TRUE(result->GetString("rule_name", &rule_name));
    if (rule_name == "SpecifyACacheValidator") {
      cache_validator_result = result;
      break;
    }
  }

  // Make sure the SpecifyACacheValidator result has the expected
  // score and impact.
  ASSERT_NE(static_cast<DictionaryValue*>(NULL), cache_validator_result);
  int rule_score = -1;
  ASSERT_TRUE(cache_validator_result->GetInteger("rule_score", &rule_score));
  ASSERT_EQ(0, rule_score);
  double rule_impact = -1.0;
  ASSERT_TRUE(cache_validator_result->GetDouble("rule_impact", &rule_impact));
  ASSERT_EQ(0.25, rule_impact);
}

TEST_F(PagespeedChromiumTest, EmptyInput) {
  std::string out, err;
  ASSERT_FALSE(pagespeed_chromium::RunPageSpeedRules("", &out, &err));
  ASSERT_TRUE(out.empty());
  ASSERT_EQ("Line: 1, column: 1, Root value must be an array or object.", err);

  ASSERT_FALSE(pagespeed_chromium::RunPageSpeedRules(
      "", "", "", "", kFilterName, "", false, false, &out, &err));
  ASSERT_TRUE(out.empty());
  ASSERT_EQ("could not parse HAR", err);
}

TEST_F(PagespeedChromiumTest, EmptyJsonInput) {
  std::string out, err;
  ASSERT_FALSE(pagespeed_chromium::RunPageSpeedRules("{}", &out, &err));
  ASSERT_TRUE(out.empty());
  ASSERT_EQ("Failed to extract required field(s) from input JSON.", err);

  ASSERT_FALSE(pagespeed_chromium::RunPageSpeedRules(
      "", "{}", "{}", "{}", kFilterName, "", false, false, &out, &err));
  ASSERT_TRUE(out.empty());
  ASSERT_EQ("could not parse HAR", err);
}

TEST_F(PagespeedChromiumTest, Basic) {
  std::string out, err;
  ASSERT_TRUE(pagespeed_chromium::RunPageSpeedRules(kBasicId,
                                                    kBasicHar,
                                                    kBasicDocument,
                                                    kBasicTimeline,
                                                    kFilterName,
                                                    kLocale,
                                                    false,
                                                    false,
                                                    &out,
                                                    &err));
  AssertValidResponse(out, err);
}

TEST_F(PagespeedChromiumTest, BasicSingleArgument) {
  std::string data;
  {
    scoped_ptr<DictionaryValue> root(new DictionaryValue);
    root->SetString("id", kBasicId);
    root->SetString("har", kBasicHar);
    root->SetString("document", kBasicDocument);
    root->SetString("timeline", kBasicTimeline);
    root->SetString("resource_filter", kFilterName);
    root->SetString("locale", kLocale);
    root->SetBoolean("save_optimized_content", false);
    base::JSONWriter::Write(root.get(), false, &data);
  }
  std::string out, err;
  ASSERT_TRUE(pagespeed_chromium::RunPageSpeedRules(data,
                                                    &out,
                                                    &err));
  AssertValidResponse(out, err);
}

}  // namespace
