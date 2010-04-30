// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)
//     and sligocki@google.com (Shawn Ligocki)

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_RESOURCE_MANAGER_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_RESOURCE_MANAGER_H_

#include <string>
#include "net/instaweb/util/public/string_util.h"

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

  // Creates an output resource with a generated name.  Such a
  // resource can only be meaningfully created in a deployment with
  // shared persistent storage, such as a the local disk on a
  // single-server system, or a multi-server configuration with a
  // database, network attached storage, or a shared cache such as
  // memcached.
  //
  // If this is not available in the current deployment, then NULL is returned.
  //
  // Every time this method is called, a new resource is generated.
  virtual OutputResource* GenerateOutputResource(const ContentType& type) = 0;


  // Creates an output resource where the name is provided by the rewriter.
  // The intent is to be able to derive the content from the name, for example,
  // by encoding URLs and metadata.
  //
  // This method is not dependent on shared persistent storage, and always
  // succeeds.
  //
  // This name is prepended with url_prefix for writing hrefs, and
  // file_prefix when working with the file system.
  //
  // This name is suffixed with the extension based on the content type.
  virtual OutputResource* NamedOutputResource(const StringPiece& name,
                                              const ContentType& type) = 0;


  virtual InputResource* CreateInputResource(const StringPiece& url) = 0;

  // Set up a basic header for a given content_type.
  virtual void SetDefaultHeaders(const ContentType& content_type,
                                 MetaData* header) = 0;

  // Set base directory of filesystem where resources will be found.
  virtual void set_base_dir(const StringPiece& dir) = 0;
  virtual void set_base_url(const StringPiece& url) = 0;

  virtual StringPiece file_prefix() const = 0;
  virtual StringPiece url_prefix() const = 0;
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_RESOURCE_MANAGER_H_
