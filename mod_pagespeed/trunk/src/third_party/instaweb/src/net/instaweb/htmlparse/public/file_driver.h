// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_HTMLPARSE_PUBLIC_FILE_DRIVER_H_
#define NET_INSTAWEB_HTMLPARSE_PUBLIC_FILE_DRIVER_H_

#include <string>
#include "net/instaweb/htmlparse/public/html_parser_types.h"
#include "net/instaweb/htmlparse/public/html_writer_filter.h"
#include "net/instaweb/htmlparse/public/html_parse.h"

namespace net_instaweb {

// Framework for reading an input HTML file, running it through
// a chain of HTML filters, and writing an output file.
class FileDriver {
 public:
  FileDriver(MessageHandler* message_handler, FileSystem* file_system);

  // Return the parser.  This can be used to add filters.
  HtmlParse* html_parse() { return &html_parse_; }

  // Helper function to generate an output .html filename from
  // an input filename.  Given "/a/b/c.html" returns "a/b/c.out.html".
  // Returns false if the input file does not contain a "."
  static bool GenerateOutputFilename(
      const char* infilename, std::string* outfilename);

  // Error messages are sent to the message file, true is returned
  // if the file was parsed successfully.
  bool ParseFile(const char* infilename, const char* outfilename);

 private:
  MessageHandler* message_handler_;
  HtmlParse html_parse_;
  HtmlWriterFilter html_write_filter_;
  bool write_filter_added_;
  FileSystem* file_system_;
};
}

#endif  // NET_INSTAWEB_HTMLPARSE_PUBLIC_FILE_DRIVER_H_
