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

void HtmlParse::AddEvent(HtmlEvent* event) {
  queue_.push_back(event);
  // If this is a leaf-node event, we need to set the iterator of the
  // corresponding leaf node to point to this event's position in the queue.
  // If this is an element event, then the iterators of the element will get
  // set in HtmlParse::AddElement and HtmlLexer::CloseElement, so there's no
  // need to do it here.  If this is some other kind of event, there are no
  // iterators to set.
  HtmlLeafNode* leaf = event->GetLeafNode();
  if (leaf != NULL) {
    leaf->set_iter(Last());
    assert(IsRewritable(leaf));
  }
}

HtmlCdataNode* HtmlParse::NewCdataNode(const std::string& contents) {
  HtmlCdataNode* cdata = new HtmlCdataNode(contents, queue_.end());
  nodes_.insert(cdata);
  return cdata;
}

HtmlCharactersNode* HtmlParse::NewCharactersNode(const std::string& literal) {
  HtmlCharactersNode* characters = new HtmlCharactersNode(literal,
                                                          queue_.end());
  nodes_.insert(characters);
  return characters;
}

HtmlCommentNode* HtmlParse::NewCommentNode(const std::string& contents) {
  HtmlCommentNode* comment = new HtmlCommentNode(contents, queue_.end());
  nodes_.insert(comment);
  return comment;
}

HtmlDirectiveNode* HtmlParse::NewDirectiveNode(const std::string& contents) {
  HtmlDirectiveNode* directive = new HtmlDirectiveNode(contents, queue_.end());
  nodes_.insert(directive);
  return directive;
}

HtmlElement* HtmlParse::NewElement(Atom tag) {
  HtmlElement* element = new HtmlElement(tag, queue_.end(), queue_.end());
  nodes_.insert(element);
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
      } else {
        HtmlLeafNode* leaf_node = event->GetLeafNode();
        if (leaf_node != NULL) {
          leaf_node->set_iter(queue_.end());
        }
      }
    }
    delete event;
  }
  queue_.clear();
}

bool HtmlParse::InsertElementBeforeElement(const HtmlNode* existing_node,
                                           HtmlNode* new_node) {
  return InsertElementBeforeEvent(existing_node->begin(), new_node);
}

bool HtmlParse::InsertElementAfterElement(const HtmlNode* existing_node,
                                          HtmlNode* new_node) {
  HtmlEventListIterator event = existing_node->end();
  ++event;
  return InsertElementBeforeEvent(event, new_node);
}

bool HtmlParse::InsertElementBeforeCurrent(HtmlNode* new_node) {
  if (deleted_current_) {
    message_handler_->Message(kError, "InsertElementBeforeCurrent after "
                                      "current has been deleted.");
  }
  return InsertElementBeforeEvent(current_, new_node);
}

bool HtmlParse::InsertElementBeforeEvent(const HtmlEventListIterator& event,
                                         HtmlNode* new_node) {
  bool ret = false;
  if (event != queue_.end()) {
    new_node->SynthesizeEvents(event, &queue_);
    ret = true;
  }
  return ret;
}

bool HtmlParse::DeleteElement(HtmlNode* node) {
  bool deleted = false;
  if (IsRewritable(node)) {
    bool done = false;
    // If node is an HtmlLeafNode, then begin() and end() might be the same.
    for (HtmlEventListIterator p = node->begin(); !done; ) {
      // We want to include end, so once p == end we still have to do one more
      // iteration.
      done = (p == node->end());

      // Clean up any nested elements/leaves as we get to their 'end' event.
      HtmlEvent* event = *p;
      HtmlElement* nested_element = event->GetEndElement();
      if (nested_element != NULL) {
        std::set<HtmlNode*>::iterator iter = nodes_.find(nested_element);
        assert(iter != nodes_.end());
        nodes_.erase(iter);
        delete nested_element;
      } else {
        HtmlLeafNode* leaf_node = event->GetLeafNode();
        if (leaf_node != NULL) {
          std::set<HtmlNode*>::iterator iter = nodes_.find(leaf_node);
          assert(iter != nodes_.end());
          nodes_.erase(iter);
          delete leaf_node;
        }
      }

      // Check if we're about to delete the current event.
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
    assert(nodes_.find(node) == nodes_.end());
    deleted = true;
  }
  return deleted;
}

bool HtmlParse::ReplaceNode(HtmlNode* existing_node, HtmlNode* new_node) {
  bool replaced = false;
  if (IsRewritable(existing_node)) {
    replaced = InsertElementBeforeElement(existing_node, new_node);
    assert(replaced);
    replaced = DeleteElement(existing_node);
    assert(replaced);
  }
  return replaced;
}

bool HtmlParse::IsRewritable(const HtmlNode* node) const {
  return IsInEventWindow(node->begin()) && IsInEventWindow(node->end());
}

bool HtmlParse::IsInEventWindow(const HtmlEventListIterator& iter) const {
  return iter != queue_.end();
}

void HtmlParse::ClearElements() {
  for (std::set<HtmlNode*>::iterator p = nodes_.begin(),
           e = nodes_.end(); p != e; ++p) {
    HtmlNode* node = *p;
    delete node;
  }
  nodes_.clear();
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
