// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)
//     and sligocki@google.com (Shawn Ligocki)

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_RESOURCE_MANAGER_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_RESOURCE_MANAGER_H_

#include <string>

namespace net_instaweb {

class ContentType;
class InputResource;
class MetaData;
class OutputResource;

class ResourceManager {
 public:
  virtual ~ResourceManager();

  // Created resources are managed by ResourceManager and eventually deleted
  // by ResourceManager's destructor.
  virtual OutputResource* CreateOutputResource(const ContentType& type) = 0;
  virtual InputResource* CreateInputResource(const std::string& url) = 0;

  // Set up a basic header for a given content_type.
  virtual void SetDefaultHeaders(const ContentType& content_type,
                                 MetaData* header) = 0;

  // Set base directory of filesystem where resources will be found.
  virtual void set_base_dir(const std::string& dir) = 0;
  virtual void set_base_url(const std::string& url) = 0;

  virtual const std::string& file_prefix() const = 0;
  virtual const std::string& url_prefix() const = 0;
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_RESOURCE_MANAGER_H_
