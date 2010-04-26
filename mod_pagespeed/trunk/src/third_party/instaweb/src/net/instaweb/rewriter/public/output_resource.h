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
class MetaData;
class Writer;

class OutputResource {
 public:
  virtual ~OutputResource();

  // Interface for writing the output file in chunks
  virtual bool StartWrite(MessageHandler* message_handler) = 0;
  virtual bool WriteChunk(const char* buf, size_t size,
                          MessageHandler* message_handler) = 0;
  virtual bool EndWrite(MessageHandler* message_handler) = 0;

  bool WriteChunk(const std::string& buf, MessageHandler* message_handler) {
    return WriteChunk(buf.data(), buf.size(), message_handler);
  }

  // Interface for writing the output file from a single string.
  bool Write(const std::string& content, MessageHandler* handler);

  virtual const std::string& url() const = 0;
  virtual const MetaData* metadata() const = 0;
  virtual MetaData* metadata() = 0;

  // In a scalable installation where the sprites must be kept in a
  // database, we cannot serve HTML that references new resources
  // that have not been committed yet, and committing to a database
  // may take too long to block on the HTML rewrite.  So we will want
  // to refactor this to check to see whether the desired resource is
  // already known.  For now we'll assume we can commit to serving the
  // resource during the HTML rewriter.
  virtual bool IsReadable() const = 0;
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_OUTPUT_RESOURCE_H_
