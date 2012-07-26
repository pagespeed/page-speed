// Copyright 2011 Google Inc. All Rights Reserved.
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

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/json/json_reader.h"
#include "base/scoped_ptr.h"
#include "base/values.h"
#include "pagespeed/core/dom.h"
#include "pagespeed/core/string_util.h"
#include "pagespeed/dom/json_dom.h"
#include "testing/gtest/include/gtest/gtest.h"

using pagespeed::DomDocument;
using pagespeed::DomElement;
using pagespeed::string_util::IntToString;

namespace {

class JsonDomTest : public testing::Test {
 protected:
  void Parse(const std::string& json_text) {
    std::string error_msg_out;
    scoped_ptr<Value> value(base::JSONReader::ReadAndReturnError(
      json_text,
      true,  // allow_trailing_comma
      NULL,  // error_code_out (ReadAndReturnError permits NULL here)
      &error_msg_out));

    if (value == NULL) {
      ADD_FAILURE() << "Couldn't parse JSON text: " << error_msg_out;
      return;
    }

    if (!value->IsType(Value::TYPE_DICTIONARY)) {
      ADD_FAILURE() << "JSON was not a dictionary value";
      return;
    }

    document_.reset(pagespeed::dom::CreateDocument(
        static_cast<const DictionaryValue*>(value.release())));
  }

  DomDocument* document() { return document_.get(); }

 private:
  scoped_ptr<DomDocument> document_;
};

class TagVisitor : public pagespeed::DomElementVisitor {
 public:
  TagVisitor() {}
  virtual void Visit(const DomElement& node);
  const std::vector<std::string>& tags() const { return tags_; }
 private:
  std::vector<std::string> tags_;
  DISALLOW_COPY_AND_ASSIGN(TagVisitor);
};

void TagVisitor::Visit(const DomElement& node) {
  tags_.push_back(node.GetTagName());
  scoped_ptr<DomDocument> subdoc(node.GetContentDocument());
  if (subdoc != NULL) {
    subdoc->Traverse(this);
  }
}

TEST_F(JsonDomTest, DocumentAndBaseUrls) {
  Parse("{\"documentUrl\":\"http://www.example.com/index.html\","
        " \"baseUrl\":\"http://www.example.com/\",\"elements\":[]}");
  ASSERT_FALSE(NULL == document());

  EXPECT_EQ("http://www.example.com/index.html", document()->GetDocumentUrl());
  EXPECT_EQ("http://www.example.com/", document()->GetBaseUrl());

  TagVisitor visitor;
  document()->Traverse(&visitor);
  EXPECT_EQ(0u, visitor.tags().size());
}

TEST_F(JsonDomTest, Tags) {
  Parse("{\"documentUrl\":\"http://www.example.com/index.html\","
        " \"baseUrl\":\"http://www.example.com/\",\"elements\":["
        "  {\"tag\":\"HTML\"},"
        "  {\"tag\":\"HEAD\"},"
        "  {\"tag\":\"TITLE\"},"
        "  {\"tag\":\"BODY\"},"
        "  {\"tag\":\"H1\"}"
        "]}");
  ASSERT_FALSE(NULL == document());

  TagVisitor visitor;
  document()->Traverse(&visitor);
  EXPECT_EQ(5u, visitor.tags().size());
  EXPECT_EQ("HTML", visitor.tags()[0]);
  EXPECT_EQ("HEAD", visitor.tags()[1]);
  EXPECT_EQ("TITLE", visitor.tags()[2]);
  EXPECT_EQ("BODY", visitor.tags()[3]);
  EXPECT_EQ("H1", visitor.tags()[4]);
}

TEST_F(JsonDomTest, SubDocuments) {
  Parse("{\"documentUrl\":\"http://www.example.com/index.html\","
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
        "]}");
  ASSERT_FALSE(NULL == document());

  TagVisitor visitor;
  document()->Traverse(&visitor);
  EXPECT_EQ(9u, visitor.tags().size());
  EXPECT_EQ("H1", visitor.tags()[0]);
  EXPECT_EQ("IFRAME", visitor.tags()[1]);
  EXPECT_EQ("IMG", visitor.tags()[2]);
  EXPECT_EQ("IFRAME", visitor.tags()[3]);
  EXPECT_EQ("DIV", visitor.tags()[4]);
  EXPECT_EQ("H2", visitor.tags()[5]);
  EXPECT_EQ("IFRAME", visitor.tags()[6]);
  EXPECT_EQ("P", visitor.tags()[7]);
  EXPECT_EQ("H3", visitor.tags()[8]);
}

class ImageVisitor : public pagespeed::DomElementVisitor {
 public:
  ImageVisitor() {}
  virtual void Visit(const DomElement& node);
  const std::string& output() const { return output_; }
 private:
  std::string output_;
  DISALLOW_COPY_AND_ASSIGN(ImageVisitor);
};

void ImageVisitor::Visit(const DomElement& node) {
  if (node.GetTagName() != "IMG") {
    return;
  }
  output_ += "[";
  std::string src;
  if (node.GetAttributeByName("src", &src)) {
    output_ += src;
  }
  output_ += "|";
  int size;
  if (node.GetActualWidth(&size) == DomElement::SUCCESS) {
    output_ += IntToString(size);
  }
  output_ += "x";
  if (node.GetActualHeight(&size) == DomElement::SUCCESS) {
    output_ += IntToString(size);
  }
  output_ += "|";
  bool specified;
  if (node.HasWidthSpecified(&specified) == DomElement::SUCCESS) {
    output_ += specified ? "W" : "w";
  }
  if (node.HasHeightSpecified(&specified) == DomElement::SUCCESS) {
    output_ += specified ? "H" : "h";
  }
  output_ += "]";
}

TEST_F(JsonDomTest, Attributes) {
  Parse("{\"documentUrl\":\"http://www.example.com/index.html\","
        " \"baseUrl\":\"http://www.example.com/\",\"elements\":["
        "  {\"tag\":\"IMG\", \"attrs\":{"
        "    \"src\":\"a.png\","
        "    \"width\":\"32\","
        "    \"height\":\"24\""
        "  },\"width\":32,\"height\":24},"
        "  {\"tag\":\"SPAN\"},"
        "  {\"tag\":\"IMG\", \"attrs\":{"
        "    \"src\":\"b.png\","
        "    \"height\":\"19\""
        "  },\"width\":40,\"height\":19},"
        "  {\"tag\":\"IMG\", \"attrs\":{"
        "    \"src\":\"c.png\""
        "  },\"width\":100,\"height\":80},"
        "  {\"tag\":\"IMG\", \"attrs\":{"
        "    \"src\":\"d.png\""
        "  }}"
        "]}");
  ASSERT_FALSE(NULL == document());

  ImageVisitor visitor;
  document()->Traverse(&visitor);
  EXPECT_EQ("[a.png|32x24|WH][b.png|40x19|wH][c.png|100x80|wh][d.png|x|wh]",
            visitor.output());
}

}  // namespace
