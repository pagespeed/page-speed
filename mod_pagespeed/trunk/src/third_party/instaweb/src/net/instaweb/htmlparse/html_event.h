// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_HTMLPARSE_HTML_EVENT_H_
#define NET_INSTAWEB_HTMLPARSE_HTML_EVENT_H_

#include <list>
#include "net/instaweb/htmlparse/public/html_element.h"
#include "net/instaweb/htmlparse/public/html_filter.h"
#include "net/instaweb/htmlparse/public/html_node.h"
#include <string>

namespace net_instaweb {

class HtmlEvent {
 public:
  explicit HtmlEvent(int line_number) : line_number_(line_number) {
  }
  virtual ~HtmlEvent();
  virtual void Run(HtmlFilter* filter) = 0;
  virtual void ToString(std::string* buffer) = 0;
  virtual HtmlElement* GetStartElement() { return NULL; }
  virtual HtmlElement* GetEndElement() { return NULL; }
  virtual HtmlLeafNode* GetLeafNode() { return NULL; }

  int line_number() const { return line_number_; }
 private:
  int line_number_;
};

class HtmlStartDocumentEvent: public HtmlEvent {
 public:
  explicit HtmlStartDocumentEvent(int line_number) : HtmlEvent(line_number) {}
  void Run(HtmlFilter* filter) { filter->StartDocument(); }
  void ToString(std::string* str) { *str += "StartDocument"; }
};

class HtmlEndDocumentEvent: public HtmlEvent {
 public:
  explicit HtmlEndDocumentEvent(int line_number) : HtmlEvent(line_number) {}
  void Run(HtmlFilter* filter) { filter->EndDocument(); }
  void ToString(std::string* str) { *str += "EndDocument"; }
};

class HtmlStartElementEvent: public HtmlEvent {
 public:
  HtmlStartElementEvent(HtmlElement* element, int line_number)
      : HtmlEvent(line_number),
        element_(element) {
  }
  void Run(HtmlFilter* filter) { filter->StartElement(element_); }
  void ToString(std::string* str) {
    *str += "StartElement ";
    *str += element_->tag().c_str();
  }
  virtual HtmlElement* GetStartElement() { return element_; }
 private:
  HtmlElement* element_;
};

class HtmlEndElementEvent: public HtmlEvent {
 public:
  HtmlEndElementEvent(HtmlElement* element, int line_number)
      : HtmlEvent(line_number),
        element_(element) {
  }
  void Run(HtmlFilter* filter) { filter->EndElement(element_); }
  void ToString(std::string* str) {
    *str += "EndElement ";
    *str += element_->tag().c_str();
  }
  virtual HtmlElement* GetEndElement() { return element_; }
 private:
  HtmlElement* element_;
};

class HtmlCdataEvent: public HtmlEvent {
 public:
  HtmlCdataEvent(HtmlCdataNode* cdata, int line_number)
      : HtmlEvent(line_number),
        cdata_(cdata) {
  }
  void Run(HtmlFilter* filter) { filter->Cdata(cdata_); }
  void ToString(std::string* str) {
    *str += "Cdata ";
    *str += cdata_->contents();
  }
  virtual HtmlLeafNode* GetLeafNode() { return cdata_; }
 private:
  HtmlCdataNode* cdata_;
};

class HtmlIEDirectiveEvent: public HtmlEvent {
 public:
  HtmlIEDirectiveEvent(const std::string& directive, int line_number)
      : HtmlEvent(line_number),
        directive_(directive) {
  }
  void Run(HtmlFilter* filter) { filter->IEDirective(directive_); }
  void ToString(std::string* str) {
    *str += "IEDirective ";
    *str += directive_;
  }
 private:
  std::string directive_;
};

class HtmlCommentEvent: public HtmlEvent {
 public:
  HtmlCommentEvent(HtmlCommentNode* comment, int line_number)
      : HtmlEvent(line_number),
        comment_(comment) {
  }
  void Run(HtmlFilter* filter) { filter->Comment(comment_); }
  void ToString(std::string* str) {
    *str += "Comment ";
    *str += comment_->contents();
  }
  virtual HtmlLeafNode* GetLeafNode() { return comment_; }
 private:
  HtmlCommentNode* comment_;
};

class HtmlCharactersEvent: public HtmlEvent {
 public:
  HtmlCharactersEvent(HtmlCharactersNode* characters, int line_number)
      : HtmlEvent(line_number),
        characters_(characters) {
  }
  void Run(HtmlFilter* filter) { filter->Characters(characters_); }
  void ToString(std::string* str) {
    *str += "Characters ";
    *str += characters_->contents();
  }
  virtual HtmlLeafNode* GetLeafNode() { return characters_; }
 private:
  HtmlCharactersNode* characters_;
};

class HtmlDirectiveEvent: public HtmlEvent {
 public:
  HtmlDirectiveEvent(HtmlDirectiveNode* directive, int line_number)
      : HtmlEvent(line_number),
        directive_(directive) {
  }
  void Run(HtmlFilter* filter) { filter->Directive(directive_); }
  void ToString(std::string* str) {
    *str += "Directive: ";
    *str += directive_->contents();
  }
  virtual HtmlLeafNode* GetLeafNode() { return directive_; }
 private:
  HtmlDirectiveNode* directive_;
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_HTMLPARSE_HTML_EVENT_H_
