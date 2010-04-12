// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_REWRITE_DRIVER_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_REWRITE_DRIVER_H_

#include <string>

namespace net_instaweb {

class HtmlParse;

class AddHeadFilter;
class BaseTagFilter;
class ResourceManager;
class CssSpriteFilter;
class OutlineFilter;
class ImgRewriteFilter;

class FileSystem;
class Hasher;
class UrlFetcher;
class Writer;
class HtmlWriterFilter;

class RewriteDriver {
 public:
  explicit RewriteDriver(HtmlParse* html_parse, FileSystem* file_system,
                         UrlFetcher* url_fetcher);
  ~RewriteDriver();

  // Adds a 'head' section to html documents if none found prior to the body.
  void AddHead();

  // Adds a filter that establishes a base tag for the document.
  // If not already present.  Base tags require a head section, and
  // so one will be added automatically if required.
  void SetBaseUrl(const char* base);

  // Adds an id based resource manager, enabling the rewriting of resources.
  // This will override any previous resource managers.
  void SetFilenameResources(const std::string& file_prefix,
                            const std::string& url_prefix,
                            int num_shards);

  // Add a hash based resource manager, enabling the rewriting of resources.
  // This will override any previous resource managers.
  void SetHashResources(const std::string& file_prefix,
                        const std::string& url_prefix,
                        int num_shards,
                        Hasher* hasher);

  // Combine CSS files in html document. This can only be called once and
  // requires SetResources to have been called.
  void SpriteCssFiles();

  // Cut out inlined styles and scripts and make them into external resources.
  // This can only be called once and requires SetResouces to have been called.
  void OutlineResources(bool outline_styles, bool outline_scripts);

  // Log encountered image urls.  Eventually rewrite them to reduce
  // file size, and possibly insert missing image sizes into img refs.
  void RewriteImages();

  // TODO(jmarantz): The purpose of exposing this member variable is to
  // allow the caller to establish the search path for resources relative
  // to requests.  This should be abstracted so that the caller doesn't
  // have to know about the resource management details.
  ResourceManager* resource_manager() const { return resource_manager_; }

  // Controls how HTML output is written.  Be sure to call this last, after
  // all other filters have been established.
  //
  // TODO(jmarantz): fix this in the implementation so that the caller can
  // install filters in any order and the writer will always be last.
  void SetWriter(Writer* writer);

 private:
  HtmlParse* html_parse_;
  AddHeadFilter* add_head_filter_;
  BaseTagFilter* base_tag_filter_;
  ResourceManager* resource_manager_;
  CssSpriteFilter* css_sprite_filter_;
  ImgRewriteFilter* img_rewrite_filter_;
  OutlineFilter* outline_filter_;
  FileSystem* file_system_;
  UrlFetcher* url_fetcher_;
  Hasher* hasher_;
  HtmlWriterFilter* html_writer_filter_;
};
}

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_REWRITE_DRIVER_H_
