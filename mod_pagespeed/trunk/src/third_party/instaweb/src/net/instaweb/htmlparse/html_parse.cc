// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "net/instaweb/htmlparse/public/html_parse.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <utility>  // for std::pair
#include "net/instaweb/htmlparse/html_event.h"
#include "net/instaweb/htmlparse/html_lexer.h"
#include "net/instaweb/htmlparse/public/html_element.h"
#include "net/instaweb/htmlparse/public/html_filter.h"
#include "net/instaweb/htmlparse/public/message_handler.h"

namespace net_instaweb {

HtmlParse::HtmlParse(MessageHandler* message_handler)
    : lexer_(new HtmlLexer(this)),
      sequence_(0),
      current_(queue_.end()),
      rewind_(false),
      message_handler_(message_handler) {
}

HtmlParse::~HtmlParse() {
  delete lexer_;
  ClearElements();
}

void HtmlParse::AddFilter(HtmlFilter* html_filter) {
  filters_.push_back(html_filter);
}

const char* HtmlParse::Intern(const std::string& new_string) {
  std::pair<StringSet::iterator, bool> p = string_table_.insert(new_string);
  return p.first->c_str();
}

const char* HtmlParse::Intern(const char* new_string) {
  std::pair<StringSet::iterator, bool> p = string_table_.insert(new_string);
  return p.first->c_str();
}

bool HtmlParse::IsInterned(const char* symbol) const {
  return (string_table_.find(symbol) != string_table_.end());
}

HtmlEventListIterator HtmlParse::Last() {
  HtmlEventListIterator p = queue_.end();
  --p;
  return p;
}

HtmlElement* HtmlParse::NewElement(const char* tag) {
  HtmlElement* element = new HtmlElement(tag, queue_.end(), queue_.end());
  elements_.insert(element);
  element->set_sequence(sequence_++);
  return element;
}

void HtmlParse::StartElement(const std::string& name,
    const std::vector<const char*>& atts) {
  HtmlElement* element = NewElement(Intern(name));
  // expect name/value pairs.
  assert(atts.size() % 2 == 0);
  for (size_t i = 0; i < atts.size(); i += 2) {
    const char* attname = Intern(atts[i]);
    const char* attvalue = atts[i + 1];
    if (attvalue != NULL) {
      attvalue = Intern(attvalue);
    }
    element->AddAttribute(attname, attvalue, "\"");
  }
  AddElement(element);
}

void HtmlParse::AddElement(HtmlElement* element) {
  HtmlStartElementEvent* event = new HtmlStartElementEvent(element);
  element_stack_.push_back(element);
  AddEvent(event);
  element->set_begin(Last());
}

HtmlElement* HtmlParse::PopElement() {
  HtmlElement* element = NULL;
  if (!element_stack_.empty()) {
    element = element_stack_.back();
    element_stack_.pop_back();
  }
  return element;
}

void HtmlParse::CloseElement(
    HtmlElement* element, HtmlElement::CloseStyle close_style) {
  HtmlEndElementEvent* end_event = new HtmlEndElementEvent(element);
  element->set_close_style(close_style);
  AddEvent(end_event);
  element->set_end(Last());
}

void HtmlParse::WarningV(
    const char* file, int line, const char *msg, va_list args) {
  message_handler_->WarningV(file, line, msg, args);
}

void HtmlParse::ErrorV(
    const char* file, int line, const char *msg, va_list args) {
  message_handler_->ErrorV(file, line, msg, args);
}

void HtmlParse::FatalErrorV(
    const char* file, int line, const char* msg, va_list args) {
  message_handler_->FatalErrorV(file, line, msg, args);
}

void HtmlParse::Error(const char* file, int line, const char* msg, ...) {
  va_list args;
  va_start(args, msg);
  ErrorV(file, line, msg, args);
  va_end(args);
}

void HtmlParse::Warning(const char* file, int line, const char* msg, ...) {
  va_list args;
  va_start(args, msg);
  WarningV(file, line, msg, args);
  va_end(args);
}

void HtmlParse::FatalError(const char* file, int line, const char* msg, ...) {
  va_list args;
  va_start(args, msg);
  FatalErrorV(file, line, msg, args);
  va_end(args);
}

void HtmlParse::StartParse(const char* url) {
  lexer_->StartParse(url);
}

void HtmlParse::FinishParse() {
  lexer_->FinishParse();
  Flush();
  ClearElements();
}

void HtmlParse::ParseText(const char* text, int size) {
  lexer_->Parse(text, size);
}

void HtmlParse::Flush() {
  for (size_t i = 0; i < filters_.size(); ++i) {
    HtmlFilter* filter = filters_[i];
    for (current_ = queue_.begin(); current_ != queue_.end(); ) {
      HtmlEvent* event = *current_;
      event->Run(filter);
      if (rewind_) {
        // Special-case iteration after deleting first element.
        current_ = queue_.begin();
        rewind_ = false;
      } else {
        ++current_;
      }
    }
    filter->Flush();
  }
  rewind_ = false;

  // Detach all the elements from their events, as we are now invalidating
  // the events, but not the elements.
  for (current_ = queue_.begin(); current_ != queue_.end(); ++current_) {
    HtmlEvent* event = *current_;
    HtmlElement* element = event->GetStartElement();
    if (element != NULL) {
      element->set_begin(queue_.end());
    } else {
      element = event->GetEndElement();
      if (element != NULL) {
        element->set_end(queue_.end());
      }
    }
    delete event;
  }
  queue_.clear();
}

void HtmlParse::InsertElementBeforeCurrent(HtmlElement* element) {
  HtmlEvent* start_element = new HtmlStartElementEvent(element);
  element->set_begin(queue_.insert(current_, start_element));
  HtmlEvent* end_element = new HtmlEndElementEvent(element);
  element->set_end(queue_.insert(current_, end_element));
  // Note that current_ is unmodified.  The current iteration will
  // continue.
}

void HtmlParse::DebugPrintQueue() {
  for (HtmlEventList::iterator p = queue_.begin(), e = queue_.end();
       p != e; ++p) {
    std::string buf;
    (*p)->ToString(&buf);
    if (p == current_) {
      fprintf(stdout, "* %s\n", buf.c_str());
    } else {
      fprintf(stdout, "  %s\n", buf.c_str());
    }
  }
}

void HtmlParse::InsertElementAfterCurrent(HtmlElement* element) {
  // This routine should only be called from inside a filter.  The
  // filters get called from Flush(), in a loop over the events,
  // which terminates at queue_.end().  So an assertion violation
  // here indicates a call that's not from a filter.
  assert(current_ != queue_.end());
  ++current_;
  InsertElementBeforeCurrent(element);
  --current_;  // past end
  --current_;  // past begin
}

bool HtmlParse::InsertElementAfterElement(HtmlElement* existing_element,
    HtmlElement* new_element) {
  bool ret = false;
  if (existing_element->end() != queue_.end()) {
    HtmlEvent* start_element = new HtmlStartElementEvent(new_element);
    HtmlEventListIterator p =
        queue_.insert(existing_element->end(), start_element);
    new_element->set_begin(p);
    HtmlEvent* end_element = new HtmlEndElementEvent(new_element);
    new_element->set_end(queue_.insert(p, end_element));
    ret = true;
  }
  return ret;
}


bool HtmlParse::DeleteElement(HtmlElement* element) {
  bool deleted = false;
  if (IsRewritable(element)) {
    bool done = false;
    // That element->begin() is the StartElementEvent, and element->end()
    // is the EndElementEvent.  Hence they cannot be equal.
    assert(element->begin() != element->end());
    for (HtmlEventListIterator p = element->begin(); !done; ) {
      done = (p == element->end());

      // Clean up any nested elements as we get to their 'end' event.
      HtmlEvent* event = *p;
      HtmlElement* nested_element = event->GetEndElement();
      if (nested_element != NULL) {
        assert(elements_.find(nested_element) != elements_.end());
        elements_.erase(nested_element);
        delete nested_element;
      }
      bool move_current = (p == current_);
      p = queue_.erase(p);
      if (move_current) {
        current_ = p;
      }
      delete event;
    }

    // Our iteration should have covered the passed-in element as well.
    assert(elements_.find(element) == elements_.end());
    deleted = true;
  }
  return deleted;
}

bool HtmlParse::IsRewritable(const HtmlElement* element) const {
  return IsInEventWindow(element->begin()) && IsInEventWindow(element->end());
}

bool HtmlParse::IsInEventWindow(const HtmlEventListIterator& iter) const {
  return iter != queue_.end();
}

void HtmlParse::ClearElements() {
  for (std::set<HtmlElement*>::iterator p = elements_.begin(),
           e = elements_.end(); p != e; ++p) {
    HtmlElement* element = *p;
    delete element;
  }
  elements_.clear();
}

bool HtmlParse::IsImplicitlyClosedTag(const char* tag) const {
  return lexer_->IsImplicitlyClosedTag(tag);
}

bool HtmlParse::TagAllowsBriefTermination(const char* tag) const {
  return lexer_->TagAllowsBriefTermination(tag);
}
}
