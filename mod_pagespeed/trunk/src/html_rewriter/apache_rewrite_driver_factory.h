// Copyright 2010 Google Inc. All Rights Reserved.
// Author: sligocki@google.com (Shawn Ligocki)

#ifndef HTML_REWRITER_APACHE_REWRITE_DRIVER_FACTORY_H_
#define HTML_REWRITER_APACHE_REWRITE_DRIVER_FACTORY_H_

#include <stdio.h>
#include "base/scoped_ptr.h"
#include "net/instaweb/rewriter/public/rewrite_driver_factory.h"

struct apr_pool_t;

namespace net_instaweb {

class CacheInterface;
class DelayController;
class FileSystem;
class Hasher;
class HtmlParse;
class HTTPCache;
class MessageHandler;
class Timer;
class UrlAsyncFetcher;
class UrlFetcher;

// Creates an Apache RewriteDriver.
class ApacheRewriteDriverFactory : public RewriteDriverFactory {
 public:
  ApacheRewriteDriverFactory();
  virtual ~ApacheRewriteDriverFactory();

 protected:
  virtual UrlFetcher* DefaultUrlFetcher();
  virtual UrlAsyncFetcher* DefaultAsyncUrlFetcher();

  // Provide defaults.
  virtual MessageHandler* NewHtmlParseMessageHandler();
  virtual FileSystem* NewFileSystem();
  virtual Hasher* NewHasher();
  virtual HtmlParse* NewHtmlParse();
  virtual Timer* NewTimer();
  virtual CacheInterface* NewCacheInterface();
  virtual AbstractMutex* NewMutex();

 private:
  apr_pool_t* pool_;
};

}  // namespace net_instaweb

#endif  // HTML_REWRITER_APACHE_REWRITE_DRIVER_FACTORY_H_
