// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)
//
// Meta-data associated with a rewriting resource.  This is
// primarily a key-value store, but additionally we want to
// get easy access to the cache expiration time.

#ifndef NET_INSTAWEB_UTIL_PUBLIC_META_DATA_H_
#define NET_INSTAWEB_UTIL_PUBLIC_META_DATA_H_

#include <vector>
#include "base/basictypes.h"
#include <string>

namespace net_instaweb {

class MessageHandler;
class Writer;

namespace HttpStatus {
// Http status codes.
// Grokked from http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html
enum Code {
  CONTINUE = 100,
  SWITCHING_PROTOCOLS = 101,

  OK = 200,
  CREATED = 201,
  ACCEPTED = 202,
  NON_AUTHORITATIVE = 203,
  NO_CONTENT = 204,
  RESET_CONTENT = 205,
  PARTIAL_CONTENT = 206,

  MULTIPLE_CHOICES = 300,
  MOVED_PERMANENTLY = 301,
  FOUND = 302,
  SEE_OTHER = 303,
  NOT_MODIFIED = 304,
  USE_PROXY = 305,
  SWITCH_PROXY = 306,  // In old spec; no longer used.
  TEMPORARY_REDIRECT = 307,

  BAD_REQUEST = 400,
  UNAUTHORIZED = 401,
  PAYMENT_REQUIRED = 402,
  FORBIDDEN = 403,
  NOT_FOUND = 404,
  METHOD_NOT_ALLOWED = 405,
  NOT_ACCEPTABLE = 406,
  PROXY_AUTH_REQUIRED = 407,
  REQUEST_TIMEOUT = 408,
  CONFLICT = 409,
  GONE = 410,
  LENGTH_REQUIRED = 411,
  PRECONDITION_FAILED = 412,
  ENTITY_TOO_LARGE = 413,
  URI_TOO_LONG = 414,
  UNSUPPORTED_MEDIA_TYPE = 415,
  RANGE_NOT_SATISFIABLE = 416,
  EXPECTATION_FAILED = 417,

  INTERNAL_SERVER_ERROR = 500,
  NOT_IMPLEMENTED = 501,
  BAD_GATEWAY = 502,
  UNAVAILABLE = 503,
  GATEWAY_TIMEOUT = 504,
  HTTP_VERSION_NOT_SUPPORTED = 505,
};
}  // namespace HttpStatus

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

  // Compute caching information.  The current time is used to compute
  // the absolute time when a cache resource will expire.  The timestamp
  // is in milliseconds since 1970.  It is an error to call any of the
  // accessors before ComputeCaching is called.
  virtual void ComputeCaching() = 0;
  virtual bool IsCacheable() const = 0;
  virtual bool IsProxyCacheable() const = 0;
  virtual int64 CacheExpirationTimeMs() const = 0;

  // Serialize meta-data to a stream.
  virtual bool Write(Writer* writer, MessageHandler* handler) const = 0;

  // Add a new header
  virtual void Add(const char* name, const char* value) = 0;

  // Parse a chunk of header text.  Returns number of bytes consumed.
  virtual int ParseChunk(const char* text, int num_bytes,
                         MessageHandler* handler) = 0;

  virtual bool headers_complete() const = 0;

  virtual int major_version() const = 0;
  virtual int minor_version() const = 0;
  virtual int status_code() const = 0;
  virtual const char* reason_phrase() const = 0;
  virtual int64 timestamp_ms() const = 0;
  virtual bool has_timestamp_ms() const = 0;

  virtual void set_major_version(const int major_version) = 0;
  virtual void set_minor_version(const int minor_version) = 0;
  virtual void set_status_code(const int status_code) = 0;
  virtual void set_reason_phrase(const std::string& reason_phrase) = 0;
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_UTIL_PUBLIC_META_DATA_H_
