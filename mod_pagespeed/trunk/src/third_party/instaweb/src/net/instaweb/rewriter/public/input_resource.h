// Copyright 2010 and onwards Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)
//
// Input resources are created by a ResourceManager. They must be able to
// read contents.

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_INPUT_RESOURCE_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_INPUT_RESOURCE_H_

#include <string>

namespace net_instaweb {

class MessageHandler;
class MetaData;

class InputResource {
 public:
  virtual ~InputResource();

  // Read complete resource, contents are stored in contents_.
  virtual bool Read(MessageHandler* message_handler) = 0;

  // Getters
  virtual const std::string& url() const = 0;
  virtual bool loaded() const = 0;  // Has file been read/loaded.
  // contents are only available when loaded()
  virtual const std::string& contents() const = 0;
  virtual const MetaData* metadata() const = 0;
};
}

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_INPUT_RESOURCE_H_
