// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_HTMLPARSE_PUBLIC_HTML_PARSER_TYPES_H_
#define NET_INSTAWEB_HTMLPARSE_PUBLIC_HTML_PARSER_TYPES_H_

#include <list>

namespace net_instaweb {

class FileSystem;
class HtmlElement;
class HtmlEvent;
class HtmlFilter;
class HtmlLexer;
class HtmlParse;
class HtmlStartElementEvent;
class HtmlWriterFilter;
class LibxmlAdapter;
class MessageHandler;
class Writer;

typedef std::list<HtmlEvent*> HtmlEventList;
typedef HtmlEventList::iterator HtmlEventListIterator;
}

#endif  // NET_INSTAWEB_HTMLPARSE_PUBLIC_HTML_PARSER_TYPES_H_
