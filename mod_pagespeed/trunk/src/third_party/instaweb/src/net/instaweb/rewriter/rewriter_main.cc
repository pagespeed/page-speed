// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)
//

#include <stdio.h>
#include <stdlib.h>
#include "net/instaweb/htmlparse/public/file_message_handler.h"
#include "net/instaweb/htmlparse/public/file_driver.h"
#include "net/instaweb/htmlparse/public/stdio_file_system.h"
#include "net/instaweb/rewriter/public/dummy_url_fetcher.h"
#include "net/instaweb/rewriter/public/resource_manager.h"
#include "net/instaweb/rewriter/public/rewrite_driver.h"

int main(int argc, char** argv) {
  int start = 1;
  net_instaweb::StdioFileSystem file_system;
  net_instaweb::DummyUrlFetcher url_fetcher;  // TODO(sligocki): make not dummy
  net_instaweb::FileMessageHandler message_handler(stderr);
  net_instaweb::FileDriver file_driver(&message_handler, &file_system);
  net_instaweb::HtmlParse* html_parse = file_driver.html_parse();
  net_instaweb::RewriteDriver rewrite_driver(html_parse, &file_system,
                                             &url_fetcher);

  if ((argc > (start + 1)) && (strcmp(argv[start], "-base") == 0)) {
    rewrite_driver.SetBase(argv[start + 1]);
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
    rewrite_driver.SpriteCssFiles();
    start += 4;
  }
  rewrite_driver.AddHead();

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

    if (file_driver.ParseFile(infile, outfile)) {
      ret = 0;  // TODO(sligocki): this is not doing anything right now.
    }
  }
  return ret;
}
