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
#include "net/instaweb/util/public/message_handler.h"

namespace net_instaweb {

HtmlParse::HtmlParse(MessageHandler* message_handler)
    : lexer_(new HtmlLexer(this)),
      sequence_(0),
      current_(queue_.end()),
      rewind_(false),
      message_handler_(message_handler),
      line_number_(1) {
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

void HtmlParse::AddElement(HtmlElement* element, int line_number) {
  HtmlStartElementEvent* event =
      new HtmlStartElementEvent(element, line_number);
  AddEvent(event);
  element->set_begin(Last());
  element->set_begin_line_number(line_number);
}


void HtmlParse::InfoV(
    const char* file, int line, const char *msg, va_list args) {
  message_handler_->InfoV(file, line, msg, args);
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

void HtmlParse::Info(const char* file, int line, const char* msg, ...) {
  va_list args;
  va_start(args, msg);
  InfoV(file, line, msg, args);
  va_end(args);
}

void HtmlParse::Warning(const char* file, int line, const char* msg, ...) {
  va_list args;
  va_start(args, msg);
  WarningV(file, line, msg, args);
  va_end(args);
}

void HtmlParse::Error(const char* file, int line, const char* msg, ...) {
  va_list args;
  va_start(args, msg);
  ErrorV(file, line, msg, args);
  va_end(args);
}

void HtmlParse::FatalError(const char* file, int line, const char* msg, ...) {
  va_list args;
  va_start(args, msg);
  FatalErrorV(file, line, msg, args);
  va_end(args);
}

void HtmlParse::InfoHere(const char* msg, ...) {
  va_list args;
  va_start(args, msg);
  InfoHereV(msg, args);
  va_end(args);
}

void HtmlParse::WarningHere(const char* msg, ...) {
  va_list args;
  va_start(args, msg);
  WarningHereV(msg, args);
  va_end(args);
}

void HtmlParse::ErrorHere(const char* msg, ...) {
  va_list args;
  va_start(args, msg);
  ErrorHereV(msg, args);
  va_end(args);
}

void HtmlParse::FatalErrorHere(const char* msg, ...) {
  va_list args;
  va_start(args, msg);
  FatalErrorHereV(msg, args);
  va_end(args);
}


void HtmlParse::StartParse(const char* url) {
  line_number_ = 1;
  filename_ = url;
  AddEvent(new HtmlStartDocumentEvent(line_number_));
  lexer_->StartParse(url);
}

void HtmlParse::FinishParse() {
  lexer_->FinishParse();
  AddEvent(new HtmlEndDocumentEvent(line_number_));
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
      line_number_ = event->line_number();
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
    line_number_ = event->line_number();
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
  HtmlEvent* start_element = new HtmlStartElementEvent(element, line_number_);
  element->set_begin(queue_.insert(current_, start_element));
  element->set_begin_line_number(line_number_);
  HtmlEvent* end_element = new HtmlEndElementEvent(element, line_number_);
  element->set_end(queue_.insert(current_, end_element));
  element->set_end_line_number(line_number_);
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
  fflush(stdout);
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
  line_number_ = (*current_)->line_number();
}

bool HtmlParse::InsertElementAfterElement(
    HtmlElement* existing_element, HtmlElement* new_element, int line_number) {
  bool ret = false;
  if (existing_element->end() != queue_.end()) {
    HtmlEvent* start_element = new HtmlStartElementEvent(
        new_element, line_number);
    HtmlEventListIterator p =
        queue_.insert(existing_element->end(), start_element);
    new_element->set_begin(p);
    new_element->set_begin_line_number(line_number);
    HtmlEvent* end_element = new HtmlEndElementEvent(new_element, line_number);
    new_element->set_end(queue_.insert(p, end_element));
    new_element->set_end_line_number(line_number);
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
        line_number_ = (*current_)->line_number();
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
