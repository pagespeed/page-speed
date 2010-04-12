// Copyright 2010 and onwards Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)

#include "net/instaweb/rewriter/public/outline_filter.h"

#include <string>
#include "net/instaweb/rewriter/public/output_resource.h"
#include "net/instaweb/rewriter/public/resource_manager.h"
#include "net/instaweb/htmlparse/public/html_parse.h"
#include "net/instaweb/htmlparse/public/html_element.h"

namespace net_instaweb {

OutlineFilter::OutlineFilter(HtmlParse* html_parse,
                             ResourceManager* resource_manager,
                             bool outline_styles,
                             bool outline_scripts)
    : inline_element_(NULL),
      html_parse_(html_parse),
      resource_manager_(resource_manager),
      outline_styles_(outline_styles),
      outline_scripts_(outline_scripts) {
  s_link_ = html_parse_->Intern("link");
  s_script_ = html_parse_->Intern("script");
  s_style_ = html_parse_->Intern("style");
  s_rel_ = html_parse_->Intern("rel");
  s_stylesheet_ = html_parse_->Intern("stylesheet");
  s_href_ = html_parse_->Intern("href");
  s_src_ = html_parse_->Intern("src");
}

void OutlineFilter::StartDocument() {
  inline_element_ = NULL;
  buffer_.clear();
}

void OutlineFilter::StartElement(HtmlElement* element) {
  // No tags allowed inside style or script element.
  if (inline_element_ != NULL) {
    // TODO(sligocki): Add negative unit tests to hit these errors.
    html_parse_->ErrorHere("Tag found inside style/script.");
    inline_element_ = NULL;  // Don't outline what we don't understand.
  }
  if (outline_styles_ && element->tag() == s_style_) {
    inline_element_ = element;
    buffer_.clear();

  } else if (outline_scripts_ && element->tag() == s_script_) {
    inline_element_ = element;
    buffer_.clear();
    // script elements which already have a src should not be outlined.
    for (int i = 0; i < element->attribute_size(); ++i) {
      if (element->attribute(i).name_ == s_src_) {
        inline_element_ = NULL;
      }
    }
  }
}

void OutlineFilter::EndElement(HtmlElement* element) {
  if (inline_element_ != NULL) {
    if (element != inline_element_) {
      // No other tags allowed inside style or script element.
      html_parse_->ErrorHere("Tag found inside style/script.");
    } else if (inline_element_->tag() == s_style_) {
      OutlineStyle(inline_element_, buffer_);
    } else if (inline_element_->tag() == s_script_) {
      OutlineScript(inline_element_, buffer_);
    } else {
      html_parse_->ErrorHere("OutlineFilter::inline_element_ not style/script");
    }
    inline_element_ = NULL;
    buffer_.clear();
  }
}

void OutlineFilter::Flush() {
  // If we were flushed in a style/script element, we cannot outline it.
  inline_element_ = NULL;
  buffer_.clear();
}


void OutlineFilter::Characters(const std::string& characters) {
  if (inline_element_ != NULL) {
    buffer_ += characters;
  }
}

void OutlineFilter::IgnorableWhitespace(const std::string& whitespace) {
  if (inline_element_ != NULL) {
    buffer_ += whitespace;
  }
}


void OutlineFilter::Comment(const std::string& comment) {
  if (inline_element_ != NULL) {
    html_parse_->ErrorHere("Comment found inside style/script.");
    inline_element_ = NULL;  // Don't outline what we don't understand.
    buffer_.clear();
  }
}

void OutlineFilter::Cdata(const std::string& cdata) {
  if (inline_element_ != NULL) {
    html_parse_->ErrorHere("CDATA found inside style/script.");
    inline_element_ = NULL;  // Don't outline what we don't understand.
    buffer_.clear();
  }
}

void OutlineFilter::IEDirective(const std::string& directive) {
  if (inline_element_ != NULL) {
    html_parse_->ErrorHere("IE Directive found inside style/script.");
    inline_element_ = NULL;  // Don't outline what we don't understand.
    buffer_.clear();
  }
}


void OutlineFilter::OutlineStyle(HtmlElement* style_element,
                                 const std::string& content) {
  if (html_parse_->IsRewritable(style_element)) {
    // Create style file from content
    // TODO(sligocki): What do we want to do if it's not CSS?
    OutputResource* resource =
        resource_manager_->CreateOutputResource(".css");
    MessageHandler* message_handler = html_parse_->message_handler();
    if (resource->Write(content, message_handler)) {
      HtmlElement* link_element = html_parse_->NewElement(s_link_);
      link_element->AddAttribute(s_rel_, s_stylesheet_, "'");
      link_element->AddAttribute(s_href_, resource->url().c_str(), "'");
      // Add all style atrributes to link.
      for (int i = 0; i < style_element->attribute_size(); ++i) {
        link_element->AddAttribute(style_element->attribute(i));
      }
      // remove style element from DOM
      if (!html_parse_->DeleteElement(style_element)) {
        html_parse_->FatalErrorHere("Failed to delete element");
      }
      // Add link
      // NOTE: this only works if current pointer was on element
      // TODO(sligocki): Do an InsertElementBeforeElement instead?
      html_parse_->InsertElementBeforeCurrent(link_element);
    }
  }
}

// TODO(sligocki): combine similar code from OutlineStyle
void OutlineFilter::OutlineScript(HtmlElement* element,
                                  const std::string& content) {
  if (html_parse_->IsRewritable(element)) {
    // Create script file from content
    // TODO(sligocki): What do we want to do if it's not javascript?
    OutputResource* resource =
        resource_manager_->CreateOutputResource(".js");
    MessageHandler* message_handler = html_parse_->message_handler();
    if (resource->Write(content, message_handler)) {
      HtmlElement* src_element = html_parse_->NewElement(s_script_);
      src_element->AddAttribute(s_src_, resource->url().c_str(), "'");
      // Add all style atrributes to link.
      for (int i = 0; i < element->attribute_size(); ++i) {
        src_element->AddAttribute(element->attribute(i));
      }
      // remove original script element from DOM
      if (!html_parse_->DeleteElement(element)) {
        html_parse_->FatalErrorHere("Failed to delete element");
      }
      // Add <script src=...> element
      // NOTE: this only works if current pointer was on element
      // TODO(sligocki): Do an InsertElementBeforeElement instead?
      html_parse_->InsertElementBeforeCurrent(src_element);
    }
  }
}
}
