// Copyright 2009 Google Inc. All Rights Reserved.
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

#include "base/scoped_ptr.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/rules/optimize_the_order_of_styles_and_scripts.h"
#include "pagespeed/testing/pagespeed_test.h"

using pagespeed::rules::OptimizeTheOrderOfStylesAndScripts;
using namespace pagespeed;
using pagespeed_testing::PagespeedRuleTest;

namespace {

class OptimizeOrderTest
    : public PagespeedRuleTest<OptimizeTheOrderOfStylesAndScripts> {
 protected:
  void AddHtmlResource(const std::string& url, const std::string& html) {
    Resource* resource = new Resource;
    resource->SetRequestUrl(url);
    resource->SetRequestMethod("GET");
    resource->SetResponseStatusCode(200);
    resource->AddResponseHeader("Content-Type", "text/html");
    resource->SetResponseBody(html);
    AddResource(resource);
  }

  void CheckOneViolation(const std::string& url,
                         int original_cpl,
                         int potential_cpl,
                         std::vector<std::string>& ooo_external_css,
                         std::vector<int>& ooo_inline_scripts) {
    Freeze();
    ASSERT_TRUE(AppendResults());

    ASSERT_EQ(1, num_results());
    ASSERT_EQ("OptimizeTheOrderOfStylesAndScripts", results_rule_name());
    const Result& result0 = result(0);
    ASSERT_EQ(1, result0.resource_urls_size());
    ASSERT_EQ(url, result0.resource_urls(0));
    ASSERT_EQ(original_cpl, result0.original_critical_path_length());

    ASSERT_TRUE(result0.has_savings());
    const Savings& savings = result0.savings();
    ASSERT_EQ(original_cpl - potential_cpl,
              savings.critical_path_length_saved());

    ASSERT_TRUE(result0.has_details());
    const ResultDetails& details = result0.details();
    ASSERT_TRUE(details.HasExtension(
        ResourceOrderingDetails::message_set_extension));
    const ResourceOrderingDetails& ordering_details =
      details.GetExtension(ResourceOrderingDetails::message_set_extension);

    ASSERT_EQ(static_cast<int>(ooo_external_css.size()),
              ordering_details.out_of_order_external_css_size());
    for (int i = 0, size = ooo_external_css.size(); i < size; ++i) {
      ASSERT_EQ(ooo_external_css[i],
                ordering_details.out_of_order_external_css(i));
    }

    ASSERT_EQ(static_cast<int>(ooo_inline_scripts.size()),
              ordering_details.out_of_order_inline_scripts_size());
    for (int i = 0, size = ooo_inline_scripts.size(); i < size; ++i) {
      ASSERT_EQ(ooo_inline_scripts[i],
                ordering_details.out_of_order_inline_scripts(i));
    }
  }
};

TEST_F(OptimizeOrderTest, Empty) {
  AddHtmlResource("http://example.com/foo.html",
                  "\n");
  CheckNoViolations();
}

TEST_F(OptimizeOrderTest, MostlyEmpty) {
  AddHtmlResource("http://example.com/foo.html",
                  "<html><head>\n"
                  "</head><body>foo</body></html>\n");
  CheckNoViolations();
}

TEST_F(OptimizeOrderTest, SimpleCssViolation) {
  AddHtmlResource("http://example.com/foo.html",
                  "<script src=\"j1.js\"></script>\n"
                  "<link rel=stylesheet href=\"c1.css\">\n"
                  "<body>foo\n");
  std::vector<std::string> ooo_external_css;
  ooo_external_css.push_back("c1.css");
  std::vector<int> ooo_inline_scripts;
  CheckOneViolation("http://example.com/foo.html", 2, 1,
                    ooo_external_css, ooo_inline_scripts);
}

TEST_F(OptimizeOrderTest, SimpleScriptViolation) {
  AddHtmlResource("http://example.com/foo.html",
                  "<link rel=stylesheet href=\"c1.css\">\n"
                  "<script>document.write('baz')</script>\n"
                  "<link rel=stylesheet href=\"c2.css\">\n"
                  "<body>foo\n");
  std::vector<std::string> ooo_external_css;
  std::vector<int> ooo_inline_scripts;
  ooo_inline_scripts.push_back(1);
  CheckOneViolation("http://example.com/foo.html", 2, 1,
                    ooo_external_css, ooo_inline_scripts);
}

TEST_F(OptimizeOrderTest, DoNotComplainAboutStyleInBody) {
  AddHtmlResource("http://example.com/foo.html",
                  "<script src=\"j1.js\"></script>\n"
                  "<body>foo\n"
                  "<link rel=stylesheet href=\"c1.css\">\n");
  CheckNoViolations();
}

TEST_F(OptimizeOrderTest, NoViolations) {
  AddHtmlResource("http://example.com/foo.html",
                  "<html><head>\n"
                  "<link rel=stylesheet href=\"c1.css\">\n"
                  "<link rel=stylesheet href=\"c2.css\">\n"
                  "<link rel=stylesheet href=\"c3.css\">\n"
                  "<script src=\"j1.js\"></script>\n"
                  "<script src=\"j2.js\"></script>\n"
                  "<script src=\"j3.js\"></script>\n"
                  "<script>document.write('baz')</script>\n"
                  "<script>document.write('quux')</script>\n"
                  "</head><body>foo</body></html>\n");
  CheckNoViolations();
}

TEST_F(OptimizeOrderTest, CssOutOfOrder) {
  AddHtmlResource("http://example.com/foo.html",
                  "<html><head>\n"
                  "<link rel=stylesheet href=\"c1.css\">\n"
                  "<link rel=stylesheet href=\"c2.css\">\n"
                  "<script src=\"j1.js\"></script>\n"
                  "<script src=\"j2.js\"></script>\n"
                  "<script src=\"j3.js\"></script>\n"
                  "<link rel=stylesheet href=\"c3.css\">\n"
                  "<script>document.write('baz')</script>\n"
                  "<script>document.write('quux')</script>\n"
                  "</head><body>foo</body></html>\n");
  std::vector<std::string> ooo_external_css;
  ooo_external_css.push_back("c3.css");
  std::vector<int> ooo_inline_scripts;
  CheckOneViolation("http://example.com/foo.html", 4, 3,
                    ooo_external_css, ooo_inline_scripts);
}

TEST_F(OptimizeOrderTest, MultipleCssOutOfOrder) {
  AddHtmlResource("http://example.com/foo.html",
                  "<html><head>\n"
                  "<link rel=stylesheet href=\"c1.css\">\n"
                  "<script src=\"j1.js\"></script>\n"
                  "<script src=\"j2.js\"></script>\n"
                  "<link rel=stylesheet href=\"c2.css\">\n"
                  "<script src=\"j3.js\"></script>\n"
                  "<link rel=stylesheet href=\"c3.css\">\n"
                  "<script>document.write('baz')</script>\n"
                  "<script>document.write('quux')</script>\n"
                  "</head><body>foo</body></html>\n");
  std::vector<std::string> ooo_external_css;
  ooo_external_css.push_back("c2.css");
  ooo_external_css.push_back("c3.css");
  std::vector<int> ooo_inline_scripts;
  CheckOneViolation("http://example.com/foo.html", 4, 3,
                    ooo_external_css, ooo_inline_scripts);
}

TEST_F(OptimizeOrderTest, InlineScriptOutOfOrder) {
  AddHtmlResource("http://example.com/foo.html",
                  "<html><head>\n"
                  "<link rel=stylesheet href=\"c1.css\">\n"
                  "<link rel=stylesheet href=\"c2.css\">\n"
                  "<script>document.write('baz')</script>\n"
                  "<link rel=stylesheet href=\"c3.css\">\n"
                  "<script src=\"j1.js\"></script>\n"
                  "<script src=\"j2.js\"></script>\n"
                  "<script src=\"j3.js\"></script>\n"
                  "<script>document.write('quux')</script>\n"
                  "</head><body>foo</body></html>\n");
  std::vector<std::string> ooo_external_css;
  std::vector<int> ooo_inline_scripts;
  ooo_inline_scripts.push_back(1);
  CheckOneViolation("http://example.com/foo.html", 4, 3,
                    ooo_external_css, ooo_inline_scripts);
}

TEST_F(OptimizeOrderTest, MultipleInlineScriptsOutOfOrder) {
  AddHtmlResource("http://example.com/foo.html",
                  "<html><head>\n"
                  "<link rel=stylesheet href=\"c1.css\">\n"
                  "<link rel=stylesheet href=\"c2.css\">\n"
                  "<script>document.write('baz')</script>\n"
                  "<link rel=stylesheet href=\"c3.css\">\n"
                  "<script>document.write('quux')</script>\n"
                  "<script src=\"j1.js\"></script>\n"
                  "<script src=\"j2.js\"></script>\n"
                  "<script src=\"j3.js\"></script>\n"
                  "</head><body>foo</body></html>\n");
  std::vector<std::string> ooo_external_css;
  std::vector<int> ooo_inline_scripts;
  ooo_inline_scripts.push_back(1);
  ooo_inline_scripts.push_back(2);
  CheckOneViolation("http://example.com/foo.html", 5, 3,
                    ooo_external_css, ooo_inline_scripts);
}

TEST_F(OptimizeOrderTest, InlineScriptAtBeginningIsOkay) {
  AddHtmlResource("http://example.com/foo.html",
                  "<html><head>\n"
                  "<script>document.write('baz')</script>\n"
                  "<link rel=stylesheet href=\"c1.css\">\n"
                  "<link rel=stylesheet href=\"c2.css\">\n"
                  "<link rel=stylesheet href=\"c3.css\">\n"
                  "<script src=\"j1.js\"></script>\n"
                  "<script src=\"j2.js\"></script>\n"
                  "<script src=\"j3.js\"></script>\n"
                  "<script>document.write('quux')</script>\n"
                  "</head><body>foo</body></html>\n");
  CheckNoViolations();
}

TEST_F(OptimizeOrderTest, NontrivialCriticalPathLength) {
  AddHtmlResource("http://example.com/foo.html",
                  "<html><head>\n"
                  "<script src=\"j1.js\"></script>\n"
                  "<link rel=stylesheet href=\"c1.css\">\n"
                  "<link rel=stylesheet href=\"c2.css\">\n"
                  "<script src=\"j2.js\"></script>\n"
                  "<link rel=stylesheet href=\"c3.css\">\n"
                  "<script>document.write('baz')</script>\n"   // inline #1
                  "<link rel=stylesheet href=\"c4.css\">\n"
                  "<link rel=stylesheet href=\"c5.css\">\n"
                  "<link rel=stylesheet href=\"c6.css\">\n"
                  "<script src=\"j3.js\"></script>\n"
                  "<script src=\"j4.js\"></script>\n"
                  "<script src=\"j5.js\"></script>\n"
                  "<script>document.write('quux')</script>\n"  // inline #2
                  "<script src=\"j6.js\"></script>\n"
                  "<link rel=stylesheet href=\"c7.css\">\n"
                  "<script>document.write('quux')</script>\n"  // inline #3
                  "<script src=\"j7.js\"></script>\n"
                  "</head><body>foo</body></html>\n");
  std::vector<std::string> ooo_external_css;
  ooo_external_css.push_back("c1.css");
  ooo_external_css.push_back("c2.css");
  ooo_external_css.push_back("c3.css");
  ooo_external_css.push_back("c4.css");
  ooo_external_css.push_back("c5.css");
  ooo_external_css.push_back("c6.css");
  ooo_external_css.push_back("c7.css");
  std::vector<int> ooo_inline_scripts;
  ooo_inline_scripts.push_back(1);
  ooo_inline_scripts.push_back(3);
  CheckOneViolation("http://example.com/foo.html", 9, 7,
                    ooo_external_css, ooo_inline_scripts);
}

}  // namespace
