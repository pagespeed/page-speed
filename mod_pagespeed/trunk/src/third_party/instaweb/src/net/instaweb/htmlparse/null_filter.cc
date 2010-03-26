// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include <stdio.h>
#include "net/instaweb/htmlparse/public/file_driver.h"
#include "net/instaweb/htmlparse/public/file_message_handler.h"
#include "net/instaweb/htmlparse/public/stdio_file_system.h"

int null_filter(int argc, char** argv) {
  int ret = 1;

  if ((argc != 2) && (argc != 3)) {
    fprintf(stdout, "Usage: %s input_file [- | output_file]\n", argv[0]);
  } else {
    net_instaweb::StdioFileSystem file_system;
    net_instaweb::FileMessageHandler message_handler(stderr);
    net_instaweb::FileDriver file_driver(&message_handler, &file_system);
    const char* infile = argv[1];
    const char* outfile = NULL;
    std::string outfile_buffer;

    if (argc == 3) {
      outfile = argv[2];
    } else  if (file_driver.GenerateOutputFilename(infile, &outfile_buffer)) {
      outfile = outfile_buffer.c_str();
      fprintf(stdout, "Null rewriting %s into %s\n", infile, outfile);
    } else {
      fprintf(stderr, "Cannot generate output filename from %s\n", infile);
    }

    if (outfile != NULL) {
      if (file_driver.ParseFile(infile, outfile)) {
        ret = 0;
      }
    }
  }

  return ret;
}
