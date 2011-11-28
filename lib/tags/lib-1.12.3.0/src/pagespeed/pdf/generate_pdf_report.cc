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

#include "pagespeed/pdf/generate_pdf_report.h"

#include <string>

#include "base/basictypes.h"
#include "base/logging.h"
#include "pagespeed/proto/pagespeed_proto_formatter.pb.h"
#include "third_party/libharu/include/hpdf.h"

namespace pagespeed {

namespace {

// Page layout constants:
const double kInch = 72.0;  // libharu's default scale is 72 DPI
const double kWidth = 7.0 * kInch, kHeight = 9.5 * kInch;
const double kLeft = 0.75 * kInch, kBottom = 0.75 * kInch;
const double kRight = kLeft + kWidth, kTop = kBottom + kHeight;
// Paragraph text constants:
const double kTextFontSize = 10.0;
const double kLineSpacing = 12.0;

class PdfGenerator {
 public:
  PdfGenerator();
  ~PdfGenerator();

  // Return true if an error has occurred so far.
  bool error() { return error_; }

  // Generate a PDF from the results and store it internally.
  void GeneratePdf(const FormattedResults& results);
  // Save the previously generated PDF to a file.
  void SaveToFile(const std::string& path);

 private:
  static void ErrorHandler(HPDF_STATUS error, HPDF_STATUS detail,
                           void* generator);
  // Append a new page to the document and set page_ to point to it.
  void NewPage();
  // Generate and append results for a single rule to the current document.
  void GenerateRuleSummary(const FormattedRuleResults& results);
  // Utility method for formatting a FormatString as a paragraph of text.  left
  // and right are the bounds of the paragraph, and baseline is the vertical
  // position at which to start the paragraph.
  void FormatParagraph(const FormatString& format_string,
                       double left, double right, double* baseline);
  // Utility method used by FormatParagraph() for wrapping a segment of text
  // within a paragraph.
  void WrapText(const std::string& text, double left, double right,
                double* cursor_x, double* baseline);
  // Utility method used by FormatParagraph().  Similar to WrapText(), but
  // embeds a hyperlink to the URL into the PDF.
  void WrapUrl(const std::string& url, double left, double right,
               double* cursor_x, double* baseline);
  // Move baseline downwards by a distance of amount; if that goes beyond the
  // end of the current page, create a new page and set baseline to the top of
  // that page.
  void AdvanceBaseline(double amount, double* baseline);

  HPDF_Doc pdf_;
  HPDF_Page page_;  // the current page (initialized by NewPage())
  HPDF_Font font_;  // for now, we just use one font everywhere
  bool error_;

  DISALLOW_COPY_AND_ASSIGN(PdfGenerator);
};

PdfGenerator::PdfGenerator()
    : error_(false) {
  pdf_ = HPDF_New(ErrorHandler, this);
  font_ = HPDF_GetFont(pdf_, "Helvetica", "StandardEncoding");
}

PdfGenerator::~PdfGenerator() {
  // HPDF_Free() deletes all resources associated with the document, so there's
  // no need to delete page_ or font_.
  HPDF_Free(pdf_);
}

void PdfGenerator::GeneratePdf(const FormattedResults& results) {
  HPDF_SetCompressionMode(pdf_, HPDF_COMP_ALL);
  for (int index = 0; index < results.rule_results_size(); ++index) {
    const FormattedRuleResults& rule_results = results.rule_results(index);
    if (rule_results.url_blocks_size() > 0) {
      GenerateRuleSummary(rule_results);
    }
  }
}

void PdfGenerator::SaveToFile(const std::string& path) {
  HPDF_SaveToFile(pdf_, path.c_str());
}

void PdfGenerator::ErrorHandler(HPDF_STATUS error_no, HPDF_STATUS detail_no,
                                void* generator) {
  CHECK(generator != NULL);
  static_cast<PdfGenerator*>(generator)->error_ = true;
  LOG(ERROR) << "Error in PdfGenerator.  error_no=" << error_no
             << " detail_no=" << detail_no;
}

void PdfGenerator::NewPage() {
  page_ = HPDF_AddPage(pdf_);
  HPDF_Page_SetSize(page_, HPDF_PAGE_SIZE_LETTER, HPDF_PAGE_PORTRAIT);
}

void PdfGenerator::GenerateRuleSummary(const FormattedRuleResults& results) {
  NewPage();

  // Make the page header.
  HPDF_Page_SetRGBFill(page_,
                       static_cast<HPDF_REAL>(0.895),
                       static_cast<HPDF_REAL>(0.922),
                       static_cast<HPDF_REAL>(0.973));
  HPDF_Page_Rectangle(page_, kLeft, kTop - 0.5 * kInch,
                      kWidth, 0.5 * kInch);
  HPDF_Page_Fill(page_);

  HPDF_Page_SetRGBStroke(page_,
                         static_cast<HPDF_REAL>(0.199),
                         static_cast<HPDF_REAL>(0.398),
                         static_cast<HPDF_REAL>(0.797));
  HPDF_Page_SetLineWidth(page_, 1.0);
  HPDF_Page_MoveTo(page_, kLeft, kTop);
  HPDF_Page_LineTo(page_, kRight, kTop);
  HPDF_Page_Stroke(page_);

  HPDF_Page_SetGrayFill(page_, 0.0);
  HPDF_Page_BeginText(page_);
  HPDF_Page_SetFontAndSize(page_, font_, 0.3 * kInch);
  HPDF_Page_TextOut(page_, kLeft + 0.1 * kInch, kTop - 0.35 * kInch,
                    "Page Speed");
  HPDF_Page_SetFontAndSize(page_, font_, 0.2 * kInch);
  HPDF_Page_TextOut(page_, kLeft + 0.1 * kInch, kTop - 0.75 * kInch,
                    results.localized_rule_name().c_str());
  HPDF_Page_EndText(page_);

  // Emit the rule summary.
  double baseline = kTop - 1.2 * kInch;
  for (int index1 = 0; index1 < results.url_blocks_size(); ++index1) {
    const FormattedUrlBlockResults& block = results.url_blocks(index1);
    FormatParagraph(block.header(), kLeft + 0.1 * kInch, kRight, &baseline);
    for (int index2 = 0; index2 < block.urls_size(); ++index2) {
      const FormattedUrlResult& url = block.urls(index2);
      AdvanceBaseline(1.5 * kLineSpacing, &baseline);
      HPDF_Page_Circle(page_, kLeft + 0.34 * kInch,
                       baseline + 0.4 * kTextFontSize, 0.02 * kInch);
      HPDF_Page_Fill(page_);
      FormatParagraph(url.result(), kLeft + 0.4 * kInch, kRight, &baseline);
    }
    AdvanceBaseline(2.5 * kLineSpacing, &baseline);
  }
}

void PdfGenerator::FormatParagraph(const FormatString& format_string,
                                   double left, double right,
                                   double* baseline) {
  const std::string& format = format_string.format();
  std::string buffer;
  double cursor_x = left;
  size_t start_index = 0;
  HPDF_Page_SetFontAndSize(page_, font_, kTextFontSize);
  HPDF_Page_SetGrayFill(page_, 0.0);
  while (start_index < format.size()) {
    // Find the next format arg.
    size_t dollar_index = format.find('$', start_index);
    if (dollar_index == std::string::npos ||
        dollar_index > format.size() - 2) {
      dollar_index = format.size();
    }
    // Buffer the text up to that point.
    buffer.append(format.data() + start_index, dollar_index - start_index);
    // If we've reached the end of the format string, we're done.
    if (dollar_index >= format.size()) {
      break;
    }
    // Format the argument.
    const char arg_number = format[dollar_index + 1];
    if (arg_number >= '1' && arg_number <= '9') {
      const FormatArgument& argument = format_string.args(arg_number - '1');
      if (argument.type() == FormatArgument::URL) {
        if (!buffer.empty()) {
          WrapText(buffer, left, right, &cursor_x, baseline);
          buffer.clear();
        }
        HPDF_Page_GSave(page_);
        HPDF_Page_SetGrayFill(page_, static_cast<HPDF_REAL>(0.4));
        WrapUrl(argument.string_value(), left, right, &cursor_x, baseline);
        HPDF_Page_GRestore(page_);
      } else {
        buffer.append(argument.localized_value());
      }
    }
    start_index = dollar_index + 2;
  }
  if (!buffer.empty()) {
    WrapText(buffer, left, right, &cursor_x, baseline);
  }
}

void PdfGenerator::WrapText(const std::string& text,
                            double left, double right,
                            double* cursor_x, double* baseline) {
  size_t start = 0;
  while (true) {
    // Measure the text to determine how much will fit on this line.
    HPDF_REAL text_width;
    const int text_offset = HPDF_Font_MeasureText(
        font_,  // font
        reinterpret_cast<const HPDF_BYTE*>(text.data() + start),  // text ptr
        text.size() - start,  // text length
        right - *cursor_x,  // available width
        kTextFontSize,  // font size
        0.0,  // char spacing
        0.0,  // word spacing
        true,  // wordwrap
        &text_width);  // actual width of substring
    // Draw the text that will fit.
    if (text_offset > 0) {
      HPDF_Page_BeginText(page_);
      const std::string line(text.substr(start, text_offset));
      HPDF_Page_TextOut(page_, *cursor_x, *baseline, line.c_str());
      HPDF_Page_EndText(page_);
    }
    // If we weren't able to write the next word even from the beginning of a
    // new line, we're hosed.
    if (text_offset == 0 && *cursor_x == left) {
      // TODO(mdsteele): A better thing to do here would be to run MeasureText
      // again with wordwrap=false, and try again, allowing us to split the
      // long word across lines.  But...that'd make things more complicated,
      // and I don't think it will ever actually happen in practice.
      LOG(ERROR) << "Single word too wide to fit on one line: "
                 << text.substr(start);
      break;
    }
    // Advance through the string to where we left off.  If we've reached the
    // end of the string, we're done.
    start += text_offset;
    if (start >= text.size()) {
      *cursor_x += text_width;
      break;
    }
    // There's more of the string left, so move to the next line of text.
    *cursor_x = left;
    AdvanceBaseline(kLineSpacing, baseline);
  }
}

void PdfGenerator::WrapUrl(const std::string& url,
                           double left, double right,
                           double* cursor_x, double* baseline) {
  std::string url_text = url;
  if (url_text.size() > 80) {
    url_text.resize(80);
    url_text.append("...");
  }
  // Measure the text to determine how much will fit on this line.
  HPDF_REAL text_width;
  const int text_offset = HPDF_Font_MeasureText(
      font_,  // font
      reinterpret_cast<const HPDF_BYTE*>(url_text.data()),  // text ptr
      url_text.size(),  // text length
      right - *cursor_x,  // available width
      kTextFontSize,  // font size
      0.0,  // char spacing
      0.0,  // word spacing
      true,  // wordwrap
      &text_width);  // actual width of substring
  if (text_offset == 0) {
    *cursor_x = left;
    AdvanceBaseline(kLineSpacing, baseline);
  }
  HPDF_Page_BeginText(page_);
  HPDF_Page_TextOut(page_, *cursor_x, *baseline, url_text.c_str());
  HPDF_Page_EndText(page_);
  const HPDF_Rect rect = {
    static_cast<HPDF_REAL>(*cursor_x),
    static_cast<HPDF_REAL>(*baseline),
    static_cast<HPDF_REAL>(*cursor_x + text_width),
    static_cast<HPDF_REAL>(*baseline + kTextFontSize)
  };
  HPDF_Page_CreateURILinkAnnot(page_, rect, url.c_str());
  *cursor_x += text_width;
}

void PdfGenerator::AdvanceBaseline(double amount, double* baseline) {
  // Remember, positive Y is up (not down, as it is in many other computer
  // graphics libraries), so we need to _subtract_ amount from baseline.
  *baseline -= amount;
  if (*baseline < kBottom) {
    NewPage();
    *baseline = kTop - kTextFontSize;
  }
}

}  // namespace

bool GeneratePdfReportToFile(const FormattedResults& results,
                             const std::string& path) {
  PdfGenerator generator;
  generator.GeneratePdf(results);
  generator.SaveToFile(path);
  return !generator.error();
}

}  // namespace pagespeed
