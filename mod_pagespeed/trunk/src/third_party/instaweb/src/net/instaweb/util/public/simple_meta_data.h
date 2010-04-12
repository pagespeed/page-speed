// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)
//
// Meta-data associated with a rewriting resource.  This is
// primarily a key-value store, but additionally we want to

#ifndef NET_INSTAWEB_UTIL_PUBLIC_SIMPLE_META_DATA_H_
#define NET_INSTAWEB_UTIL_PUBLIC_SIMPLE_META_DATA_H_

#include <map>
#include <string>
#include <vector>
#include "net/instaweb/util/public/meta_data.h"

namespace net_instaweb {

// Very basic implementation of HTTP headers.
//
// TODO(jmarantz): implement caching rules properly.
// TODO(jmarantz): implement case insensitivity.
class SimpleMetaData : public MetaData {
 public:
  SimpleMetaData();
  virtual ~SimpleMetaData();

  // Raw access for random access to attribute name/value pairs
  virtual int NumAttributes() const;
  virtual const char* Name(int index) const;
  virtual const char* Value(int index) const;

  // Get the attribute values associated with this name.  Returns
  // false if the attribute is not found.  If it was found, then
  // the values vector is filled in.
  virtual bool Lookup(const char* name, StringVector* values) const;

  // Specific information about cache.  This is all embodied in the
  // headers but is centrally parsed so we can try to get it right.
  virtual bool IsCacheable() const;
  virtual bool IsProxyCacheable() const;

  // Returns the seconds-since-1970 absolute time when this resource
  // should be expired out of caches.
  virtual int64 CacheExpirationTime() const;

  // Serialize meta-data to a stream.
  virtual bool Write(Writer* writer, MessageHandler* message_handler) const;

  // Add a new header
  virtual void Add(const char* name, const char* value);

  // Parse a chunk of header text.  Returns number of bytes consumed.
  virtual int ParseChunk(const char* text, int num_bytes,
                         MessageHandler* handler);

  virtual bool headers_complete() const { return headers_complete_; }
  virtual int status_code() const { return status_code_; }
  virtual const char* reason_phrase() const {
    return reason_phrase_.c_str();
  }
  virtual int major_version() const { return major_version_; }
  virtual int minor_version() const { return minor_version_; }

 private:
  bool GrabLastToken(const std::string& input, std::string* output);

  // We are keeping two structures, conseptually map<String,vector<String>> and
  // vector<pair<String,String>>, so we can do associative lookups and
  // also order-preserving iteration and random access.
  //
  // To avoid duplicating the strings, we will have the map own the
  // Names (keys) in a std::string, and the string-pair-vector own the
  // value as an explicitly newed char*.  The risk of using a std::string
  // to hold the value is that the pointers will not survive a resize.
  typedef std::pair<const char*, char*> StringPair;  // owns the value
  typedef std::map<std::string, StringVector> AttributeMap;  // owns the key
  AttributeMap attribute_map_;
  std::vector<StringPair> attribute_vector_;
  bool parsing_http_;
  bool parsing_value_;
  bool headers_complete_;
  std::string parse_name_;
  std::string parse_value_;
  int major_version_;
  int minor_version_;
  int status_code_;
  std::string reason_phrase_;
};
}

#endif  // NET_INSTAWEB_UTIL_PUBLIC_SIMPLE_META_DATA_H_
