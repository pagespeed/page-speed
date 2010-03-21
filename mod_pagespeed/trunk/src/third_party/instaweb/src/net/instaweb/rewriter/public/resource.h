// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_RESOURCE_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_RESOURCE_H_

#include <string>

namespace net_instaweb {
class FileSystem;
class MessageHandler;
class Writer;

class Resource {
 public:
  virtual ~Resource();

  // TODO(sligocki): Should this be Read?
  virtual bool Load(
      FileSystem* file_system, MessageHandler* message_handler) = 0;
  virtual bool IsLoaded() const = 0;
  virtual bool Write(
      FileSystem* file_system, Writer* fn, MessageHandler* message_handler) = 0;

};
}

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_RESOURCE_H_
