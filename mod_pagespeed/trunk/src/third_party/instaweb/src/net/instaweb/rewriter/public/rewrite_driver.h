// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_REWRITE_DRIVER_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_REWRITE_DRIVER_H_

namespace net_instaweb {

class HtmlParse;
class AddHeadFilter;
class BaseTagFilter;
class FileResourceManager;
class CssSpriteFilter;
class OutlineFilter;
class FileSystem;

class RewriteDriver {
 public:
  explicit RewriteDriver(HtmlParse* html_parse, FileSystem* file_system);
  ~RewriteDriver();

  // Adds a 'head' section to html documents if none found prior to the body.
  void AddHead();

  // Adds a filter that establishes a base tag for the document.
  // If not already present.  Base tags require a head section, and
  // so one will be added automatically if required.
  void SetBase(const char* base);

  // Adds a resource manager, enabling the rewriting of resources.  This
  // can only be called once.
  void SetResources(
      const char* file_prefix, const char* serving_prefix, int num_shards);

  // Adds a resource manager, enabling the rewriting of resources.  This
  // can only be called once, and requires SetResources to have been called.
  void SpriteCssFiles();

  void OutlineResources();

  // TODO(jmarantz): The purpose of exposing this member variable is to
  // allow the caller to establish the search path for resources relative
  // to requests.  This should be abstracted so that the caller doesn't
  // have to know about the resource management details.
  FileResourceManager* file_resource_manager() const {
    return file_resource_manager_;
  }

 private:
  HtmlParse* html_parse_;
  AddHeadFilter* add_head_filter_;
  BaseTagFilter* base_tag_filter_;
  FileResourceManager* file_resource_manager_;
  CssSpriteFilter* css_sprite_filter_;
  OutlineFilter* outline_filter_;
  FileSystem* file_system_;
};
}

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_REWRITE_DRIVER_H_
