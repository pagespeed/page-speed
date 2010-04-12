// Copyright 2010 Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)
//
// UrlFetcher is an interface for fetching urls.
//
// TODO(jmarantz): we must extend this interface to specify user-agent
// coming in, and capture caching information coming out.
//
// TODO(jmarantz): Don't return the entire fetched contents in this call.
// Intead, allow the caller to specify a Writer so it can work with the
// data without copying it.
//
// TODO(jmarantz): Consider asynchronous fetches.  This may not require
// a change in interface; we would simply always return 'false' if the
// url contents is not already cached.  We may want to consider a richer
// return-value enum to distinguish illegal ULRs from invalid ones, from
// ones where the fetch is in-progress.  Or maybe the caller doesn't care.

#ifndef NET_INSTAWEB_UTIL_PUBLIC_URL_FETCHER_H_
#define NET_INSTAWEB_UTIL_PUBLIC_URL_FETCHER_H_

#include <string>

namespace net_instaweb {

class MessageHandler;
class MetaData;
class Writer;

class UrlFetcher {
 public:
  virtual ~UrlFetcher();

  // Fetch a URL, streaming the output to fetched_content_writer, and
  // returning the headers.  The headers must be deleted by the caller.
  // request_headers is optional -- it can be NULL.  If the fetch is
  // not successful, NULL is returned.
  virtual MetaData* StreamingFetchUrl(const std::string& url,
                                      const MetaData* request_headers,
                                      Writer* fetched_content_writer,
                                      MessageHandler* message_handler) = 0;

  // Convenience method for fetching URL into a string, with no headers in
  // our out.  This is primarily for upward compatibility.
  //
  // TODO(jmarantz): change callers to use StreamingFetchUrl and remove this.
  bool FetchUrl(const std::string& url,
                std::string* content,
                MessageHandler* message_handler);
};
}

#endif  // NET_INSTAWEB_UTIL_PUBLIC_URL_FETCHER_H_
