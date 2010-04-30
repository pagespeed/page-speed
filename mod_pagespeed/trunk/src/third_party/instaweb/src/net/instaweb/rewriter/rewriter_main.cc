// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)
//

#include <stdio.h>
#include <stdlib.h>
#include "base/scoped_ptr.h"
#include "net/instaweb/htmlparse/public/file_driver.h"
#include "net/instaweb/rewriter/public/filename_resource_manager.h"
#include "net/instaweb/rewriter/public/rewrite_driver.h"
#include "net/instaweb/util/public/cache_url_fetcher.h"
#include "net/instaweb/util/public/fake_url_async_fetcher.h"
#include "net/instaweb/util/public/file_message_handler.h"
#include "net/instaweb/util/public/filename_encoder.h"
#include "net/instaweb/util/public/http_cache.h"
#include "net/instaweb/util/public/lru_cache.h"
#include "net/instaweb/util/public/stdio_file_system.h"
#include "net/instaweb/util/public/timer.h"
#include "net/instaweb/util/public/wget_url_fetcher.h"

int main(int argc, char** argv) {
  int start = 1;
  net_instaweb::StdioFileSystem file_system;
  net_instaweb::WgetUrlFetcher url_fetcher;
  net_instaweb::FileMessageHandler message_handler(stderr);
  net_instaweb::FileDriver file_driver(&message_handler, &file_system);
  net_instaweb::HtmlParse* html_parse = file_driver.html_parse();
  net_instaweb::LRUCache lru_cache(100 * 1000 * 1000);
  scoped_ptr<net_instaweb::Timer> timer(net_instaweb::Timer::NewSystemTimer());
  net_instaweb::HTTPCache http_cache(&lru_cache, timer.get());
  net_instaweb::CacheUrlFetcher cache_url_fetcher(&http_cache, &url_fetcher);
  net_instaweb::FakeUrlAsyncFetcher url_async_fetcher(&cache_url_fetcher);
  net_instaweb::RewriteDriver rewrite_driver(html_parse, &url_async_fetcher);

  scoped_ptr<net_instaweb::ResourceManager> manager;
  if ((argc > (start + 4)) &&
      (strcmp(argv[start], "-resource_patterns") == 0)) {
    const char* file_prefix = argv[start + 1];
    const char* serving_prefix = argv[start + 2];
    const char* num_shards_string = argv[start + 3];
    char* endptr;
    int num_shards = strtol(num_shards_string, &endptr, 10);  // NOLINT
    if ((num_shards < 0) || (num_shards > 100)) {
      fprintf(stderr, "invalid number of shards: %d, stay between 0 and 100\n",
              num_shards);
      return 1;
    }
    net_instaweb::FilenameEncoder filename_encoder;
    bool write_headers = false;
    manager.reset(new net_instaweb::FilenameResourceManager(
        file_prefix, serving_prefix, num_shards, write_headers, &file_system,
        &filename_encoder, &url_fetcher));
    rewrite_driver.SetResourceManager(manager.get());
    start += 4;
  }

  if ((argc > (start + 1)) && (strcmp(argv[start], "-combine_css") == 0)) {
    rewrite_driver.CombineCssFiles();
    start += 1;
  }

  bool outline_styles = false;
  bool outline_scripts = false;
  if ((argc > (start + 1)) && (strcmp(argv[start], "-outline_styles") == 0)) {
    outline_styles = true;
    start += 1;
  }
  if ((argc > (start + 1)) && (strcmp(argv[start], "-outline_scripts") == 0)) {
    outline_scripts = true;
    start += 1;
  }
  if (outline_styles || outline_scripts) {
    rewrite_driver.OutlineResources(outline_styles, outline_scripts);
  }

  if ((argc > (start + 1)) && (strcmp(argv[start], "-rewrite_imgs") == 0)) {
    rewrite_driver.RewriteImages();
    start += 1;
  }

  if ((argc > (start + 1)) && (strcmp(argv[start], "-remove_quotes") == 0)) {
    rewrite_driver.RemoveQuotes();
    start += 1;
  }

  const char* infile = argv[start];
  const char* outfile = NULL;
  std::string outfile_buffer;

  if (argc - start == 2) {
    outfile = argv[start + 1];
  } else  if (file_driver.GenerateOutputFilename(infile, &outfile_buffer)) {
    outfile = outfile_buffer.c_str();
    fprintf(stdout, "Rewriting %s into %s\n", infile, outfile);
  } else {
    fprintf(stderr, "Cannot generate output filename from %s\n", infile);
  }

  int ret = 0;
  if (outfile != NULL) {
    if (manager != NULL) {
      const char* last_slash = strrchr(infile, '/');
      if (last_slash != NULL) {
        // set the input search directory for the file resource manager.
        // note that this is distinct from any 'base' that is supplied
        // for the serving side.
        std::string base_dir;
        int num_chars_before_slash = last_slash - infile;
        base_dir.append(infile, num_chars_before_slash);
        std::string file_url("file:/");
        if (infile[0] != '/') {
          file_url += getenv("PWD");
          file_url += '/';
        }
        char* last_slash = strrchr(infile, '/');
        if (last_slash != NULL) {
          int base_len = last_slash - infile + 1;
          file_url.append(infile, base_len);
        }
        rewrite_driver.SetBaseUrl(file_url.c_str());
      }
    }

    // TODO(jmaessen): generate stats from rewriting?
    if (file_driver.ParseFile(infile, outfile, NULL)) {
      ret = 0;  // TODO(sligocki): this is not doing anything right now.
    }
  }
  return ret;
}
