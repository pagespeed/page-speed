// Copyright 2010 Google Inc. All Rights Reserved.
// Author: sligocki@google.com (Shawn Ligocki)

#ifndef HTML_REWRITER_APACHE_REWRITE_DRIVER_FACTORY_H_
#define HTML_REWRITER_APACHE_REWRITE_DRIVER_FACTORY_H_

#include <stdio.h>
#include <set>
#include <vector>
#include "base/scoped_ptr.h"
#include "net/instaweb/rewriter/public/rewrite_driver_factory.h"

struct apr_pool_t;

namespace net_instaweb {

// Creates an Apache RewriteDriver.
class ApacheRewriteDriverFactory : public RewriteDriverFactory {
 public:
  ApacheRewriteDriverFactory();
  virtual ~ApacheRewriteDriverFactory();

  RewriteDriver* GetRewriteDriver();
  void ReleaseRewriteDriver(RewriteDriver* rewrite_driver);

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
  virtual AbstractMutex* cache_mutex() { return cache_mutex_.get(); }
  virtual AbstractMutex* rewrite_drivers_mutex() {
    return rewrite_drivers_mutex_.get(); }

 private:
  apr_pool_t* pool_;
  scoped_ptr<AbstractMutex> cache_mutex_;
  scoped_ptr<AbstractMutex> rewrite_drivers_mutex_;
  std::vector<RewriteDriver*> rewrite_drivers_;
  std::set<RewriteDriver*> active_rewrite_drivers_;
};

}  // namespace net_instaweb

#endif  // HTML_REWRITER_APACHE_REWRITE_DRIVER_FACTORY_H_
