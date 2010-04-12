// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)
//
// Meta-data associated with a rewriting resource.  This is
// primarily a key-value store, but additionally we want to

#include "net/instaweb/util/public/simple_meta_data.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "net/instaweb/util/public/message_handler.h"
#include "net/instaweb/util/public/writer.h"

namespace net_instaweb {

SimpleMetaData::SimpleMetaData()
    : parsing_http_(false),
      parsing_value_(false),
      headers_complete_(false),
      major_version_(0),
      minor_version_(0),
      status_code_(0) {
}

SimpleMetaData::~SimpleMetaData() {
  for (int i = 0, n = attribute_vector_.size(); i < n; ++i) {
    delete [] attribute_vector_[i].second;
  }
}

int SimpleMetaData::NumAttributes() const {
  return attribute_vector_.size();
}

const char* SimpleMetaData::Name(int index) const {
  return attribute_vector_[index].first;
}

const char* SimpleMetaData::Value(int index) const {
  return attribute_vector_[index].second;
}

bool SimpleMetaData::Lookup(const char* name, StringVector* values) const {
  // TODO(jmarantz): implement case insensitivity
  AttributeMap::const_iterator p = attribute_map_.find(name);
  bool ret = false;
  if (p != attribute_map_.end()) {
    ret = true;
    *values = p->second;
  }
  return ret;
}

// Specific information about cache.  This is all embodied in the
// headers but is centrally parsed so we can try to get it right.
bool SimpleMetaData::IsCacheable() const {
  MetaData::StringVector values;
  return Lookup("cache-control", &values);
}

bool SimpleMetaData::IsProxyCacheable() const {
  // TODO(jmarantz): Re-implement correctly.

  MetaData::StringVector values;
  bool ret = Lookup("cache-control", &values);
  if (ret) {
    const char* cache_control = values[values.size() - 1];
    ret = (strstr(cache_control, "private") == NULL);
  }
  return ret;
}

// Returns the seconds-since-1970 absolute time when this resource
// should be expired out of caches.
int64 SimpleMetaData::CacheExpirationTime() const {
  // TODO(jmarantz): Re-implement correctly.  In particular, bmcquade sez:
  // the computation would be DateHeader+max_age (so you need to use Date header
  // as the base for the computation.  If no Date header is specified you can
  // use the response time (but not the current time)....you also need to look
  // at the expiration header if max-age isnt present.
  int64 ret = 0;
  StringVector values;
  if (Lookup("cache-control", &values)) {
    // const char* cache_control = LookupValue("cache-control", count - 1);
    // TODO(jmarantz): parse the cache-control string.
    ret = 5;
  }
  return ret;
}

// Serialize meta-data to a stream.
bool SimpleMetaData::Write(Writer* writer, MessageHandler* handler) const {
  bool ret = true;
  char buf[100];
  snprintf(buf, sizeof(buf), "HTTP/%d.%d %d ",
           major_version_, minor_version_, status_code_);
  writer->Write(buf, strlen(buf), handler);
  writer->Write(reason_phrase_.data(), reason_phrase_.size(), handler);
  writer->Write("\r\n", 2, handler);
  for (int i = 0, n = attribute_vector_.size(); ret && (i < n); ++i) {
    const StringPair& attribute = attribute_vector_[i];
    ret &= writer->Write(attribute.first, strlen(attribute.first), handler);
    ret &= writer->Write(": ", 2, handler);
    ret &= writer->Write(attribute.second, strlen(attribute.second),
                         handler);
    ret &= writer->Write("\r\n", 2, handler);
  }
  ret &= writer->Write("\r\n", 2, handler);
  return ret;
}

void SimpleMetaData::Add(const char* name, const char* value) {

  // TODO(jmarantz): Parse comma-separated values.  bmcquade sez:
  // you probably want to normalize these by splitting on commas and
  // adding a separate k,v pair for each comma-separated value. then
  // it becomes very easy to do things like search for individual
  // Content-Type tokens. Otherwise the client has to assume that
  // every single value could be comma-separated and they have to
  // parse it as such.  the list of header names that are not safe to
  // comma-split is at
  // http://src.chromium.org/viewvc/chrome/trunk/src/net/http/http_util.cc
  // (search for IsNonCoalescingHeader)

  StringVector dummy_values;
  std::pair<AttributeMap::iterator, bool> iter_inserted =
      attribute_map_.insert(AttributeMap::value_type(name, dummy_values));
  AttributeMap::iterator iter = iter_inserted.first;
  StringVector& values = iter->second;
  int value_buf_size = strlen(value) + 1;
  char* value_copy = new char[value_buf_size];
  memcpy(value_copy, value, value_buf_size);
  values.push_back(value_copy);
  attribute_vector_.push_back(StringPair(iter->first.c_str(), value_copy));
}

int SimpleMetaData::ParseChunk(const char* text, int num_bytes,
                               MessageHandler* handler) {
  assert(!headers_complete_);
  int num_consumed = 0;

  for (; num_consumed < num_bytes; ++num_consumed) {
    char c = text[num_consumed];
    if ((c == '/') && (parse_name_ == "HTTP")) {
      if (major_version_ != 0) {
        handler->Error("???", 0, "Multiple HTTP Lines");
      } else {
        parsing_http_ = true;
        parsing_value_ = true;
      }
    } else if (!parsing_value_ && (c == ':')) {
      parsing_value_ = true;
    } else if (c == '\r') {
      // Just ignore CRs for now, and break up headers on newlines for
      // simplicity.  It's not clear to me if it's important that we
      // reject headers that lack the CR in front of the LF.
    } else if (c == '\n') {
      if (parse_name_.empty()) {
        // blank line.  This marks the end of the headers.
        ++num_consumed;
        headers_complete_ = true;
        break;
      }
      if (parsing_http_) {
        // Parsing "1.0 200 OK\r", using sscanf for the integers, and
        // private method GrabLastToken for the "OK".
        if ((sscanf(parse_value_.c_str(), "%d.%d %d ",  // NOLINT
                    &major_version_, &minor_version_, &status_code_) != 3) ||
            !GrabLastToken(parse_value_, &reason_phrase_)) {
          // TODO(jmarantz): capture the filename/url, track the line numbers.
          handler->Error("???", 0, "Invalid HTML headers: %s",
                         parse_value_.c_str());
        }
        parsing_http_ = false;
      } else {
        Add(parse_name_.c_str(), parse_value_.c_str());
      }
      parsing_value_ = false;
      parse_name_.clear();
      parse_value_.clear();
    } else if (parsing_value_) {
      // Skip leading whitespace
      if (!parse_value_.empty() || !isspace(c)) {
        parse_value_ += c;
      }
    } else {
      parse_name_ += c;
    }
  }
  return num_consumed;
}

// Grabs the last non-whitespace token from 'input' and puts it in 'output'.
bool SimpleMetaData::GrabLastToken(const std::string& input,
                                   std::string* output) {
  bool ret = false;
  // Safely grab the response code string from the end of parse_value_.
  int last_token_char = -1;
  for (int i = input.size() - 1; i >= 0; --i) {
    char c = input[i];
    if (isspace(c)) {
      if (last_token_char >= 0) {
        // We found the whole token.
        const char* token_start = input.c_str() + i + 1;
        int token_len = last_token_char - i;
        output->append(token_start, token_len);
        ret = true;
        break;
      }
    } else if (last_token_char == -1) {
      last_token_char = i;
    }
  }
  return ret;
}
}
