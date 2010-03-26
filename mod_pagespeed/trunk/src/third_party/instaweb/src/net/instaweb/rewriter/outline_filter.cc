// Copyright 2010 and onwards Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)

#include "net/instaweb/rewriter/public/outline_filter.h"

#include <assert.h>
#include <string>
#include "net/instaweb/rewriter/public/output_resource.h"
#include "net/instaweb/rewriter/public/resource_manager.h"
#include "net/instaweb/htmlparse/public/html_parse.h"
#include "net/instaweb/htmlparse/public/html_element.h"

namespace net_instaweb {

OutlineFilter::OutlineFilter(HtmlParse* html_parse,
                             ResourceManager* resource_manager)
    : html_parse_(html_parse),
      resource_manager_(resource_manager) {
  s_link_ = html_parse_->Intern("link");
  s_script_ = html_parse_->Intern("script");
  s_style_ = html_parse_->Intern("style");
  s_rel_ = html_parse_->Intern("rel");
  s_stylesheet_ = html_parse_->Intern("stylesheet");
  s_href_ = html_parse_->Intern("href");
  s_src_ = html_parse_->Intern("src");
  inline_element_ = NULL;
}

void OutlineFilter::StartDocument() {
  inline_element_ = NULL;
  buffer_.clear();
  element_stack_.clear();
}

void OutlineFilter::StartElement(HtmlElement* element) {
  // No tags allowed inside style or script element.
  // TODO(sligocki): This should eventually not be a fatal error.
  if (inline_element_ != NULL) {
    html_parse_->FatalError(html_parse_->filename(), html_parse_->line_number(),
                            "Tag found inside style/script.");
    inline_element_ = NULL;  // Don't outline what we don't understand.
  }
  element_stack_.push_back(element);
  if (element->tag() == s_style_ || element->tag() == s_script_) {
    inline_element_ = element;
    buffer_.clear();
    // script elements which already have a src should not be outlined.
    if (element->tag() == s_script_) {
      for (int i = 0; i < element->attribute_size(); ++i) {
        if (element->attribute(i).name_ == s_src_) {
          inline_element_ = NULL;
        }
      }
    }
  }
}

void OutlineFilter::EndElement(HtmlElement* element) {
  if (inline_element_ != NULL) {
    if (element->tag() == s_style_) {
      assert(element == inline_element_);
      OutlineStyle(element, buffer_);
      inline_element_ = NULL;
      buffer_.clear();
    } else if (element->tag() == s_script_) {
      assert(element == inline_element_);
      OutlineScript(element, buffer_);
      inline_element_ = NULL;
      buffer_.clear();
    } else {
      assert(false);  // No tags allowed inside style or script element.
    }
  }
  assert(element == element_stack_.back());
  element_stack_.pop_back();
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


// TODO(sligocki): What do we do with comments?
void OutlineFilter::Comment(const std::string& comment) {
  // TODO(sligocki): This should eventually not be a fatal error.
  if (inline_element_ != NULL) {
    html_parse_->FatalError(html_parse_->filename(), html_parse_->line_number(),
                            "Tag found inside style/script.");
    inline_element_ = NULL;  // Don't outline what we don't understand.
  }
}

void OutlineFilter::Cdata(const std::string& cdata) {
  // TODO(sligocki): This should eventually not be a fatal error.
  if (inline_element_ != NULL) {
    html_parse_->FatalError(html_parse_->filename(), html_parse_->line_number(),
                            "Tag found inside style/script.");
    inline_element_ = NULL;  // Don't outline what we don't understand.
  }
}

void OutlineFilter::IEDirective(const std::string& directive) {
  // TODO(sligocki): This should eventually not be a fatal error.
  if (inline_element_ != NULL) {
    html_parse_->FatalError(html_parse_->filename(), html_parse_->line_number(),
                            "Tag found inside style/script.");
    inline_element_ = NULL;  // Don't outline what we don't understand.
  }
}


void OutlineFilter::OutlineStyle(HtmlElement* style_element,
                                 const std::string& content) {
  if (html_parse_->IsRewritable(style_element)) {
    // create style file from content
    // TODO(sligocki): What do we want to do if it's not CSS?
    OutputResource* resource =
        resource_manager_->CreateOutputResource(".css");
    MessageHandler* message_handler = html_parse_->message_handler();
    if (resource->Write(content, message_handler)) {
      HtmlElement* link_element = html_parse_->NewElement(s_link_);
      link_element->AddAttribute(s_rel_, s_stylesheet_, "'");
      // TODO(sligocki): sanitize url so it doesn't have stray 's
      link_element->AddAttribute(s_href_, resource->url().c_str(), "'");
      // Add all style atrributes to link.
      for (int i = 0; i < style_element->attribute_size(); ++i) {
        link_element->AddAttribute(style_element->attribute(i));
      }
      // remove style element from DOM
      if (!html_parse_->DeleteElement(style_element)) {
        // TODO(sligocki): what do I do for first few args.
        html_parse_->FatalError(
            html_parse_->filename(), html_parse_->line_number(),
            "Failed to delete element");
      }
      // add link
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
    // create style file from content
    // TODO(sligocki): What do we want to do if it's not javascript?
    OutputResource* resource =
        resource_manager_->CreateOutputResource(".js");
    MessageHandler* message_handler = html_parse_->message_handler();
    if (resource->Write(content, message_handler)) {
      HtmlElement* src_element = html_parse_->NewElement(s_script_);
      // TODO(sligocki): sanitize url so it doesn't have stray 's
      src_element->AddAttribute(s_src_, resource->url().c_str(), "'");
      // Add all style atrributes to link.
      for (int i = 0; i < element->attribute_size(); ++i) {
        src_element->AddAttribute(element->attribute(i));
      }
      // remove original script element from DOM
      if (!html_parse_->DeleteElement(element)) {
        // TODO(sligocki): what do I do for first few args.
        html_parse_->FatalError(
            html_parse_->filename(), html_parse_->line_number(),
            "Failed to delete element");
      }
      // add <script src=...> element
      // NOTE: this only works if current pointer was on element
      // TODO(sligocki): Do an InsertElementBeforeElement instead?
      html_parse_->InsertElementBeforeCurrent(src_element);
    }
  }
}
}
