// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)
//
// Meta-data associated with a rewriting resource.  This is
// primarily a key-value store, but additionally we want to

#ifndef NET_INSTAWEB_UTIL_PUBLIC_META_DATA_H_
#define NET_INSTAWEB_UTIL_PUBLIC_META_DATA_H_

#include <vector>
#include "base/basictypes.h"

namespace net_instaweb {

class MessageHandler;
class Writer;

// Container for required meta-data.  General HTTP headers can be added
// here as name/value pairs, and caching information can then be derived.
//
// TODO(jmarantz): consider rename to HTTPHeader.
class MetaData {
 public:
  typedef std::vector<const char*> StringVector;

  virtual ~MetaData();

  // Raw access for random access to attribute name/value pairs
  virtual int NumAttributes() const = 0;
  virtual const char* Name(int index) const = 0;
  virtual const char* Value(int index) const = 0;

  // Get the attribute values associated with this name.  Returns
  // false if the attribute is not found.  If it was found, then
  // the values vector is filled in.
  virtual bool Lookup(const char* name, StringVector* values) const = 0;

  // Specific information about cache.  This is all embodied in the
  // headers but is centrally parsed so we can try to get it right.
  virtual bool IsCacheable() const = 0;
  virtual bool IsProxyCacheable() const = 0;

  // Returns the seconds-since-1970 absolute time when this resource
  // should be expired out of caches.
  virtual int64 CacheExpirationTime() const = 0;

  // Serialize meta-data to a stream.
  virtual bool Write(Writer* writer, MessageHandler* handler) const = 0;

  // Add a new header
  virtual void Add(const char* name, const char* value) = 0;

  // Parse a chunk of header text.  Returns number of bytes consumed.
  virtual int ParseChunk(const char* text, int num_bytes,
                         MessageHandler* handler) = 0;

  virtual bool headers_complete() const = 0;
  virtual int status_code() const = 0;
  virtual int major_version() const = 0;
  virtual int minor_version() const = 0;
  virtual const char* reason_phrase() const = 0;
};
}

#endif  // NET_INSTAWEB_UTIL_PUBLIC_META_DATA_H_
