// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)
//

#include <stdio.h>
#include <stdlib.h>
#include "net/instaweb/htmlparse/public/file_driver.h"
#include "net/instaweb/rewriter/public/resource_manager.h"
#include "net/instaweb/rewriter/public/rewrite_driver.h"
#include "net/instaweb/util/public/file_message_handler.h"
#include "net/instaweb/util/public/stdio_file_system.h"
#include "net/instaweb/util/public/wget_url_fetcher.h"

int main(int argc, char** argv) {
  int start = 1;
  net_instaweb::StdioFileSystem file_system;
  net_instaweb::WgetUrlFetcher url_fetcher;
  net_instaweb::FileMessageHandler message_handler(stderr);
  net_instaweb::FileDriver file_driver(&message_handler, &file_system);
  net_instaweb::HtmlParse* html_parse = file_driver.html_parse();
  net_instaweb::RewriteDriver rewrite_driver(html_parse, &file_system,
                                             &url_fetcher);

  if ((argc > (start + 1)) && (strcmp(argv[start], "-base") == 0)) {
    rewrite_driver.SetBaseUrl(argv[start + 1]);
    start += 2;
  }

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
    rewrite_driver.SetFilenameResources(file_prefix, serving_prefix,
                                        num_shards);
    start += 4;
  }

  if ((argc > (start + 1)) && (strcmp(argv[start], "-sprite_css") == 0)) {
    rewrite_driver.SpriteCssFiles();
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
    if (rewrite_driver.resource_manager() != NULL) {
      const char* last_slash = strrchr(infile, '/');
      if (last_slash != NULL) {
        // set the input search directory for the file resource manager.
        // note that this is distinct from any 'base' that is supplied
        // for the serving side.
        std::string base_dir;
        int num_chars_before_slash = last_slash - infile;
        base_dir.append(infile, num_chars_before_slash);
        rewrite_driver.resource_manager()->set_base_dir(base_dir);
      }
    }

    // TODO(jmaessen): generate stats from rewriting?
    if (file_driver.ParseFile(infile, outfile, NULL)) {
      ret = 0;  // TODO(sligocki): this is not doing anything right now.
    }
  }
  return ret;
}
