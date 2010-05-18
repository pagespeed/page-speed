// Copyright 2010 and onwards Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_FILENAME_RESOURCE_MANAGER_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_FILENAME_RESOURCE_MANAGER_H_

#include <vector>
#include "base/scoped_ptr.h"
#include "net/instaweb/rewriter/public/resource_manager.h"
#include "net/instaweb/util/public/string_util.h"

class GURL;

namespace net_instaweb {

class ContentType;
class FileSystem;
class FilenameEncoder;
class InputResource;
class OutputResource;
class MessageHandler;
class MetaData;
class UrlFetcher;

// Note: direct usage of this class is deprecated.  Use HashResourceManager,
// with a mock_hasher if you don't want any hashing to occur.
class FilenameResourceManager : public ResourceManager {
 public:
  FilenameResourceManager(FileSystem* file_system, UrlFetcher* url_fetcher);

  virtual ~FilenameResourceManager();

  virtual InputResource* CreateInputResource(const StringPiece& url,
                                             MessageHandler* handler);

  virtual void SetDefaultHeaders(const ContentType& content_type,
                                 MetaData* header);

  virtual void set_base_url(const StringPiece& url);

 protected:
  std::vector<OutputResource*> output_resources_;
 private:
  FileSystem* file_system_;
  UrlFetcher* url_fetcher_;
  scoped_ptr<GURL> base_url_;  // Base url to resolve relative urls against.
  std::vector<InputResource*> input_resources_;
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_FILENAME_RESOURCE_MANAGER_H_
