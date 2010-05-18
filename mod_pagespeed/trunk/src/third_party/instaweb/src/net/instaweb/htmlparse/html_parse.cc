// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "net/instaweb/htmlparse/public/html_parse.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <utility>  // for std::pair
#include "net/instaweb/htmlparse/html_event.h"
#include "net/instaweb/htmlparse/html_lexer.h"
#include "net/instaweb/htmlparse/public/html_element.h"
#include "net/instaweb/htmlparse/public/html_filter.h"
#include "net/instaweb/util/public/message_handler.h"
#include <string>

namespace net_instaweb {

HtmlParse::HtmlParse(MessageHandler* message_handler)
    : lexer_(new HtmlLexer(this)),
      sequence_(0),
      current_(queue_.end()),
      deleted_current_(false),
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

HtmlEventListIterator HtmlParse::Last() {
  HtmlEventListIterator p = queue_.end();
  --p;
  return p;
}

HtmlElement* HtmlParse::NewElement(Atom tag) {
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
      deleted_current_ = false;
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

bool HtmlParse::InsertElementBeforeElement(const HtmlElement* existing_element,
                                           HtmlElement* new_element) {
  return InsertElementBeforeEvent(existing_element->begin(), new_element);
}

bool HtmlParse::InsertElementAfterElement(const HtmlElement* existing_element,
                                          HtmlElement* new_element) {
  HtmlEventListIterator event = existing_element->end();
  ++event;
  return InsertElementBeforeEvent(event, new_element);
}

bool HtmlParse::InsertElementBeforeCurrent(HtmlElement* new_element) {
  if (deleted_current_) {
    message_handler_->Message(kError, "InsertElementBeforeCurrent after "
                                      "current has been deleted.");
  }
  return InsertElementBeforeEvent(current_, new_element);
}

bool HtmlParse::InsertElementBeforeEvent(const HtmlEventListIterator& event,
                                         HtmlElement* new_element) {
  bool ret = false;
  if (event != queue_.end()) {
    int line_number = -1;  // Bogus line number b/c we are adding it ourselves.
    HtmlEvent* start_tag = new HtmlStartElementEvent(new_element, line_number);
    new_element->set_begin(queue_.insert(event, start_tag));
    HtmlEvent* end_tag = new HtmlEndElementEvent(new_element, line_number);
    new_element->set_end(queue_.insert(event, end_tag));
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
        current_ = p;  // p is the event *after* the old current.
        --current_;    // Go to *previous* event so that we don't skip p.
        deleted_current_ = true;
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

bool HtmlParse::IsImplicitlyClosedTag(Atom tag) const {
  return lexer_->IsImplicitlyClosedTag(tag);
}

bool HtmlParse::TagAllowsBriefTermination(Atom tag) const {
  return lexer_->TagAllowsBriefTermination(tag);
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

}  // namespace net_instaweb
