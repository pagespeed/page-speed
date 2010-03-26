// Copyright 2010 and onwards Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)
//
// Output resources are created by a ResourceManager. They must be able to
// write contents and return their url (so that it can be href'd on a page).

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_OUTPUT_RESOURCE_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_OUTPUT_RESOURCE_H_

#include <string>

namespace net_instaweb {

class MessageHandler;

class OutputResource {
 public:
  virtual ~OutputResource();

  // Write complete resource. Multiple calls will overwrite.
  virtual bool Write(const std::string& content,
                     MessageHandler* message_handler) = 0;

  virtual const std::string& url() const = 0;
};
}

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_OUTPUT_RESOURCE_H_
