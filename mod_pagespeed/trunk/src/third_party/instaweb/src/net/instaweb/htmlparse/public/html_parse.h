// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_HTMLPARSE_PUBLIC_HTML_PARSE_H_
#define NET_INSTAWEB_HTMLPARSE_PUBLIC_HTML_PARSE_H_

#include <stdarg.h>
#include <set>
#include <string>
#include <vector>
#include "net/instaweb/htmlparse/public/html_element.h"
#include "net/instaweb/htmlparse/public/html_parser_types.h"

namespace net_instaweb {

class HtmlParse {
 public:
  explicit HtmlParse(MessageHandler* message_handler);
  ~HtmlParse();

  // Application methods for parsing functions and adding filters

  // Add a new html filter to the filter-chain
  void AddFilter(HtmlFilter* filter);

  // Initiate a chunked parsing session.  Finish with FinishParse.  The
  // url_or_filename is only used for error messages; the contents are not
  // directly fetched.  The caller must supply the text and call ParseText.
  void StartParse(const char* url_or_filename);

  // Parses an arbitrary block of an html file, queuing up the events.  Call
  // Flush to send the events through the Filter.
  //
  // To parse an entire file, first call StartParse(filename), then call
  // ParseText on the file contents (in whatever size chunks are convenient),
  // then call FinishParse().
  void ParseText(const char* content, int size);

  // Flush the currently queued events through the filters.  It is desirable
  // for large web pages, particularly dynamically generated ones, to start
  // getting delivered to the browser as soon as they are ready.  On the
  // other hand, rewriting is more powerful when more of the content can
  // be considered for image/css/js spriting.  This method should be called
  // when the controlling network process wants to induce a new chunk of
  // output.  The less you call this function the better the rewriting will
  // be.
  void Flush();

  // Finish a chunked parsing session.  This also induces a Flush.
  void FinishParse();


  // Utiliity methods for implementing filters

  // Filters can mutate Elements in-place at will.  But to insert elements
  // before or after the current element, these can be used.
  // Downstream filters will then see the new elements but the current filter
  // will not, and neither will upstream filters.
  void InsertElementBeforeCurrent(HtmlElement* element);
  void InsertElementAfterCurrent(HtmlElement* element);
  bool InsertElementAfterElement(HtmlElement* existing_element,
                                 HtmlElement* new_element);

  HtmlElement* NewElement(const char* tag);
  bool DeleteElement(HtmlElement* element);

  bool IsRewritable(const HtmlElement* element) const;

  void ClearElements();

  void DebugPrintQueue();  // Print queue (for debugging)

  const char* Intern(const std::string& name);
  const char* Intern(const char* name);
  const char* Intern(const unsigned char* s) {return Intern((const char*) s);}
  bool IsInterned(const char* name) const;

  // Implementation helper with detailed knowledge of html parsing libraries
  friend class HtmlLexer;

  // Interface for any caller to report an error message via the message handler
  void Error(const char* filename, int line, const char* msg, ...);
  void FatalError(const char* filename, int line, const char* msg, ...);
  void Warning(const char* filename, int line, const char* msg, ...);

  void WarningV(const char* file, int line, const char *msg, va_list args);
  void ErrorV(const char* file, int line, const char *msg, va_list args);
  void FatalErrorV(const char* file, int line, const char* msg, va_list args);

  MessageHandler* message_handler() const { return message_handler_; }

  // Determines whether a tag should be terminated in HTML.
  bool IsImplicitlyClosedTag(const char* tag) const;

  // Determines whether a tag allows brief termination in HTML, e.g. <tag/>
  bool TagAllowsBriefTermination(const char* tag) const;

 private:
  HtmlElement* PopElement();
  void CloseElement(HtmlElement* element, HtmlElement::CloseStyle close_style);
  void AddElement(HtmlElement* element);
  void StartElement(const std::string& name,
      const std::vector<const char*>& atts);
  void AddEvent(HtmlEvent* event) { queue_.push_back(event); }
  HtmlEventListIterator Last();  // Last element in queue
  bool IsInEventWindow(const HtmlEventListIterator& iter) const;


  typedef std::set<std::string> StringSet;

  StringSet string_table_;
  std::vector<HtmlFilter*> filters_;
  HtmlLexer* lexer_;
  int sequence_;
  std::vector<HtmlElement*> element_stack_;
  std::set<HtmlElement*> elements_;
  HtmlEventList queue_;
  HtmlEventListIterator current_;
  bool rewind_;
  MessageHandler* message_handler_;
};
}

#endif  // NET_INSTAWEB_HTMLPARSE_PUBLIC_HTML_PARSE_H_
