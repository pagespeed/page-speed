// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_HTMLPARSE_HTML_EVENT_H_
#define NET_INSTAWEB_HTMLPARSE_HTML_EVENT_H_

#include <list>
#include <string>
#include "net/instaweb/htmlparse/public/html_element.h"
#include "net/instaweb/htmlparse/public/html_filter.h"

namespace net_instaweb {

class HtmlEvent {
 public:
  virtual ~HtmlEvent();
  virtual void Run(HtmlFilter* filter) = 0;
  virtual void ToString(std::string* buffer) = 0;
  virtual HtmlElement* GetStartElement() { return NULL; }
  virtual HtmlElement* GetEndElement() { return NULL; }
};

class HtmlStartDocumentEvent: public HtmlEvent {
 public:
  void Run(HtmlFilter* filter) { filter->StartDocument(); }
  void ToString(std::string* str) { *str += "StartDocument"; }
};

class HtmlEndDocumentEvent: public HtmlEvent {
 public:
  void Run(HtmlFilter* filter) { filter->EndDocument(); }
  void ToString(std::string* str) { *str += "EndDocument"; }
};

class HtmlEndElementEvent;

class HtmlStartElementEvent: public HtmlEvent {
 public:
  explicit HtmlStartElementEvent(HtmlElement* element) : element_(element) {
  }
  void Run(HtmlFilter* filter) { filter->StartElement(element_); }
  void ToString(std::string* str) {
    *str += "StartElement ";
    *str += element_->tag();
  }
  virtual HtmlElement* GetStartElement() { return element_; }
 private:
  HtmlElement* element_;
};

class HtmlEndElementEvent: public HtmlEvent {
 public:
  explicit HtmlEndElementEvent(HtmlElement* element): element_(element) {
  }
  void Run(HtmlFilter* filter) { filter->EndElement(element_); }
  void ToString(std::string* str) {
    *str += "EndElement ";
    *str += element_->tag();
  }
  virtual HtmlElement* GetEndElement() { return element_; }
 private:
  HtmlElement* element_;
};

class HtmlCdataEvent: public HtmlEvent {
 public:
  explicit HtmlCdataEvent(const std::string& cdata) : cdata_(cdata) {
  }
  void Run(HtmlFilter* filter) { filter->Cdata(cdata_); }
  void ToString(std::string* str) {
    *str += "Cdata ";
    *str += cdata_;
  }
 private:
  std::string cdata_;
};

class HtmlIEDirectiveEvent: public HtmlEvent {
 public:
  explicit HtmlIEDirectiveEvent(const std::string& directive)
      : directive_(directive) {
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
  explicit HtmlCommentEvent(const std::string& comment)
      : comment_(comment) {
  }
  void Run(HtmlFilter* filter) { filter->Comment(comment_); }
  void ToString(std::string* str) {
    *str += "Comment ";
    *str += comment_;
  }
 private:
  std::string comment_;
};

class HtmlCharactersEvent: public HtmlEvent {
 public:
  explicit HtmlCharactersEvent(const std::string& characters)
      : characters_(characters) {
  }
  void Run(HtmlFilter* filter) { filter->Characters(characters_); }
  void ToString(std::string* str) {
    *str += "Characters ";
    *str += characters_;
  }
 private:
  std::string characters_;
};

class HtmlWhitespaceEvent: public HtmlEvent {
 public:
  explicit HtmlWhitespaceEvent(const std::string& whitespace)
      : whitespace_(whitespace) {
  }
  void Run(HtmlFilter* filter) { filter->IgnorableWhitespace(whitespace_); }
  void ToString(std::string* str) { *str += "Whitespace"; }
 private:
  std::string whitespace_;
};

class HtmlDocTypeEvent: public HtmlEvent {
 public:
  HtmlDocTypeEvent(const std::string& name,
      const std::string& ext_id, const std::string& sys_id)
    : name_(name), ext_id_(ext_id), sys_id_(sys_id) {
  }
  void Run(HtmlFilter* filter) { filter->DocType(name_, ext_id_, sys_id_); }
  void ToString(std::string* str) { *str += "DocType"; }
 private:
  std::string name_;
  std::string ext_id_;
  std::string sys_id_;
};

class HtmlDirectiveEvent: public HtmlEvent {
 public:
  explicit HtmlDirectiveEvent(const std::string& value)
      : value_(value) {
  }
  void Run(HtmlFilter* filter) { filter->Directive(value_.c_str()); }
  void ToString(std::string* str) {
    *str += "Directive: ";
    *str += value_;
  }
 private:
  std::string value_;
};

class HtmlFlushEvent: public HtmlEvent {
 public:
  void Run(HtmlFilter* filter) { filter->Flush(); }
  void ToString(std::string* str) { *str += "Flush"; }
 private:
  std::string name_;
  std::string ext_id_;
  std::string sys_id_;
};
}

#endif  // NET_INSTAWEB_HTMLPARSE_HTML_EVENT_H_
